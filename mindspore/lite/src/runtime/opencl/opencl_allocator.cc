/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/runtime/opencl/opencl_allocator.h"
#include <utility>
#include "src/runtime/opencl/opencl_runtime.h"
#include "src/runtime/kernel/opencl/utils.h"
#include "utils/log_adapter.h"
#include "include/errorcode.h"

namespace mindspore::lite::opencl {

OpenCLAllocator::OpenCLAllocator(OpenCLRuntime *ocl_runtime) : ocl_runtime_(ocl_runtime) {}

OpenCLAllocator::~OpenCLAllocator() { Clear(); }

void OpenCLAllocator::SetContext(const AllocatorContext &ctx) {
  lock_flag_ = ctx.lockFlag;
  shift_factor_ = ctx.shiftFactor;
}

void OpenCLAllocator::Lock() {
  if (lock_flag_) {
    lock.lock();
  }
}

void OpenCLAllocator::UnLock() {
  if (lock_flag_) {
    lock.unlock();
  }
}

void *OpenCLAllocator::Malloc(size_t size) { return Malloc(size, std::vector<size_t>{}); }

void *OpenCLAllocator::Malloc(size_t size, const std::vector<size_t> &img_size) {
  auto svm_capabilities = ocl_runtime_->GetSVMCapabilities();

  size_t img_pitch = 0;
  size_t dtype_size = 1;
  if (!img_size.empty()) {
    dtype_size = img_size[2] == CL_FLOAT ? sizeof(cl_float4) : sizeof(cl_half4);
    uint32_t image_alignment = ocl_runtime_->GetImagePitchAlignment();
    img_pitch = (img_size[0] + image_alignment - 1) / image_alignment * image_alignment;
    size = img_pitch * img_size[1] * dtype_size;
  }
  if (size > MAX_MALLOC_SIZE) {
    MS_LOG(ERROR) << "MallocData out of max_size, size: " << size;
    return nullptr;
  }
  Lock();
  auto iter = free_list_.lower_bound(size);
  while (iter != free_list_.end() && (iter->second->size_ >= size) && (iter->second->size_ < (size << shift_factor_))) {
    auto mem_buf = iter->second;
    bool is_match{mem_buf->img_size.size() == img_size.size()};
    for (int i = 0; i < img_size.size() && is_match; ++i) {
      is_match &= img_size[i] == mem_buf->img_size[i];
    }
    if (is_match) {
      free_list_.erase(iter);
      allocated_list_[mem_buf->host_ptr_] = mem_buf;
      UnLock();
      MS_LOG(DEBUG) << "Malloc Image2D from free list. size: " << mem_buf->size_
                    << ", host addr: " << mem_buf->host_ptr_ << ", device addr: " << mem_buf->device_ptr_;
      return mem_buf->host_ptr_;
    }
    ++iter;
  }
  void *host_ptr = nullptr;
  void *device_ptr = nullptr;
  void *image_ptr = nullptr;
  cl::Buffer *buffer = nullptr;
  cl::Image2D *image = nullptr;

  if (svm_capabilities) {
    cl_svm_mem_flags flags = (svm_capabilities & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) ? CL_MEM_SVM_FINE_GRAIN_BUFFER : 0;
    flags |= (svm_capabilities & CL_DEVICE_SVM_ATOMICS) ? CL_MEM_SVM_ATOMICS : 0;
    flags = flags | CL_MEM_READ_WRITE;
    host_ptr = clSVMAlloc((*ocl_runtime_->Context())(), flags, size, 0);
  } else {
    cl_int ret = CL_SUCCESS;
    buffer = new (std::nothrow)
      cl::Buffer(*ocl_runtime_->Context(), CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, size, NULL, &ret);
    if (buffer == nullptr || ret != CL_SUCCESS) {
      UnLock();
      MS_LOG(ERROR) << "Create OpenCL buffer failed! (ERROR CODE: " << ret << ")";
      return nullptr;
    }
    device_ptr = static_cast<void *>(buffer);
    host_ptr = ocl_runtime_->MapBuffer(*buffer, CL_MAP_READ | CL_MAP_WRITE, size);
    if (host_ptr == nullptr) {
      delete buffer;
      UnLock();
      MS_LOG(ERROR) << "Map buffer failed, can not found buffer :" << device_ptr << ", host_ptr=" << host_ptr;
      return nullptr;
    }
    cl::Memory *mem = buffer;
    ocl_runtime_->UnmapBuffer(*mem, host_ptr);
    if (!img_size.empty()) {
      cl::ImageFormat image_format(CL_RGBA, img_size[2]);
      image = new (std::nothrow) cl::Image2D(*ocl_runtime_->Context(), image_format, *buffer, img_size[0], img_size[1],
                                             img_pitch * dtype_size, &ret);
      if (image == nullptr || ret != CL_SUCCESS) {
        delete buffer;
        UnLock();
        MS_LOG(ERROR) << "Create OpenCL Image2D failed! (ERROR CODE: " << ret << ")";
        return nullptr;
      }
      MS_LOG(DEBUG) << "Malloc a new Image2D, width=" << img_size[0] << ", height=" << img_size[1];
      image_ptr = static_cast<void *>(image);
    }
  }
  MemBuf *mem_buf = new (std::nothrow) MemBuf;
  if (mem_buf == nullptr) {
    delete buffer;
    delete image;
    return nullptr;
  }
  mem_buf->size_ = size;
  mem_buf->device_ptr_ = device_ptr;
  mem_buf->host_ptr_ = host_ptr;
  mem_buf->image_ptr_ = image_ptr;
  mem_buf->img_size = img_size;
  std::string type_name = img_size.empty() ? "buffer" : "Image2D";
  allocated_list_[host_ptr] = mem_buf;
  UnLock();
  MS_LOG(DEBUG) << "Malloc a new " << type_name << ". size: " << mem_buf->size_ << ", host addr: " << mem_buf->host_ptr_
                << ", device addr: " << mem_buf->device_ptr_ << ", image_addr: " << image_ptr;
  return host_ptr;
}

void *OpenCLAllocator::CreateImageFromHost(void *data, size_t size, const std::vector<size_t> &img_size) {
  if (size > MAX_MALLOC_SIZE) {
    MS_LOG(ERROR) << "MallocData out of max_size, size: " << size;
    return nullptr;
  }
  Lock();
  auto iter = free_list_.lower_bound(size);
  while (iter != free_list_.end() && (iter->second->size_ >= size) && (iter->second->size_ < (size << shift_factor_))) {
    auto mem_buf = iter->second;
    bool is_match{mem_buf->img_size.size() == img_size.size()};
    for (int i = 0; i < img_size.size() && is_match; ++i) {
      is_match &= img_size[i] == mem_buf->img_size[i];
    }
    if (is_match) {
      free_list_.erase(iter);
      allocated_list_[mem_buf->host_ptr_] = mem_buf;
      UnLock();
      MS_LOG(DEBUG) << "Malloc Image2D from free list. size: " << mem_buf->size_
                    << ", host addr: " << mem_buf->host_ptr_ << ", device addr: " << mem_buf->device_ptr_;
      return mem_buf->host_ptr_;
    }
    ++iter;
  }
  void *host_ptr = nullptr;
  void *device_ptr = nullptr;
  void *image_ptr = nullptr;
  cl_int ret = CL_SUCCESS;
  // CL_HALF_FLOAT, CL_FLOAT
  cl::ImageFormat image_format(CL_RGBA, img_size[2]);
  cl::Image2D *image = new (std::nothrow) cl::Image2D(*ocl_runtime_->Context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                      image_format, img_size[0], img_size[1], 0, data, &ret);
  if (image == nullptr || ret != CL_SUCCESS) {
    MS_LOG(ERROR) << "Create OpenCL Image2D failed! (ERROR CODE: " << ret << ")";
    UnLock();
    delete image;
    return nullptr;
  }
  image_ptr = static_cast<void *>(image);
  std::vector<size_t> region{img_size[0], img_size[1], 1};
  host_ptr = ocl_runtime_->MapBuffer(*image, 0, CL_MAP_READ | CL_MAP_WRITE, region);
  if (host_ptr == nullptr) {
    delete image;
    UnLock();
    MS_LOG(ERROR) << "Map buffer failed, can not found buffer :" << device_ptr << ", host_ptr=" << host_ptr;
    return nullptr;
  }
  cl::Memory *mem = image;
  ocl_runtime_->UnmapBuffer(*mem, host_ptr);
  MemBuf *mem_buf = new (std::nothrow) MemBuf;
  if (mem_buf == nullptr) {
    delete image;
    return nullptr;
  }
  mem_buf->size_ = size;
  mem_buf->device_ptr_ = device_ptr;
  mem_buf->image_ptr_ = image_ptr;
  mem_buf->host_ptr_ = host_ptr;
  mem_buf->img_size = img_size;
  allocated_list_[host_ptr] = mem_buf;
  UnLock();
  MS_LOG(DEBUG) << "Malloc a new Image2D. size: " << mem_buf->size_ << ", host addr: " << mem_buf->host_ptr_
                << ", device addr: " << mem_buf->device_ptr_ << ", image addr: " << mem_buf->image_ptr_;
  return host_ptr;
}
void OpenCLAllocator::Free(void *buf) {
  if (buf == nullptr) {
    return;
  }
  Lock();
  auto iter = allocated_list_.find(buf);
  if (iter != allocated_list_.end()) {
    if (iter->second->map_flags) {
      UnmapBuffer(buf);
      iter->second->map_flags = false;
    }
    auto mem_buf = iter->second;
    allocated_list_.erase(iter);
    free_list_.insert(std::make_pair(mem_buf->size_, mem_buf));
    UnLock();
    MS_LOG(DEBUG) << "Free device buffer. size: " << mem_buf->size_ << ", host addr: " << mem_buf->host_ptr_
                  << ", device addr: " << mem_buf->device_ptr_ << ", image addr: " << mem_buf->image_ptr_
                  << ", free list size: " << free_list_.size();
    return;
  }
  UnLock();
  MS_LOG(WARNING) << "Host ptr " << buf << " has freed";
}

size_t OpenCLAllocator::GetTotalSize() {
  Lock();
  size_t totalSize = 0;

  for (auto it = allocated_list_.begin(); it != allocated_list_.end(); it++) {
    totalSize += it->second->size_;
  }

  for (auto it = free_list_.begin(); it != free_list_.end(); it++) {
    totalSize += it->second->size_;
  }
  UnLock();
  return totalSize;
}

void *OpenCLAllocator::GetImage(void *buffer) {
  auto it = allocated_list_.find(buffer);
  if (it != allocated_list_.end()) {
    return it->second->image_ptr_;
  }
  return nullptr;
}

void *OpenCLAllocator::GetBuffer(void *buffer) {
  auto it = allocated_list_.find(buffer);
  if (it != allocated_list_.end()) {
    return it->second->device_ptr_;
  }
  return nullptr;
}

void OpenCLAllocator::Clear() {
  Lock();
  auto svm_capabilities = ocl_runtime_->GetSVMCapabilities();
  for (auto it = allocated_list_.begin(); it != allocated_list_.end(); it++) {
    if (it->second->map_flags) {
      UnmapBuffer(it->second->host_ptr_);
    }
    if (svm_capabilities) {
      clSVMFree((*ocl_runtime_->Context())(), it->second->host_ptr_);
      MS_LOG(DEBUG) << "OpenCL free svm buffer : " << it->second->host_ptr_;
    } else {
      cl::Buffer *buffer = static_cast<cl::Buffer *>(it->second->device_ptr_);
      MS_LOG(DEBUG) << "OpenCL free device buffer : " << buffer;
      if (buffer != nullptr) {
        delete buffer;
        it->second->device_ptr_ = nullptr;
      }
      cl::Image *image = static_cast<cl::Image *>(it->second->image_ptr_);
      if (image != nullptr) {
        delete image;
        it->second->image_ptr_ = nullptr;
      }
    }
    delete it->second;
  }
  allocated_list_.clear();

  for (auto it = free_list_.begin(); it != free_list_.end(); it++) {
    if (svm_capabilities) {
      clSVMFree((*ocl_runtime_->Context())(), it->second->host_ptr_);
      MS_LOG(DEBUG) << "OpenCL free svm buffer : " << it->second->host_ptr_;
    } else {
      cl::Buffer *buffer = static_cast<cl::Buffer *>(it->second->device_ptr_);
      if (buffer != nullptr) {
        MS_LOG(DEBUG) << "OpenCL free device buffer : " << buffer;
        delete buffer;
        it->second->device_ptr_ = nullptr;
      }
      cl::Image *image = static_cast<cl::Image *>(it->second->image_ptr_);
      if (image != nullptr) {
        MS_LOG(DEBUG) << "OpenCL free image : " << image;
        delete image;
        it->second->image_ptr_ = nullptr;
      }
    }
    delete it->second;
  }
  free_list_.clear();
  UnLock();
}

void *OpenCLAllocator::MapBuffer(void *host_ptr, int flags, void *command_queue, bool sync) {
  auto svm_capabilities = ocl_runtime_->GetSVMCapabilities();
  if (svm_capabilities) {
    if (!(svm_capabilities & CL_DEVICE_SVM_FINE_GRAIN_BUFFER)) {
      auto it = allocated_list_.find(host_ptr);
      if (it == allocated_list_.end()) {
        MS_LOG(ERROR) << "Map buffer failed, can not found buffer :" << host_ptr;
        return nullptr;
      }
      ocl_runtime_->MapBuffer(host_ptr, flags, it->second->size_, static_cast<cl::CommandQueue *>(command_queue), sync);
    }
    return host_ptr;
  }
  Lock();
  auto it = allocated_list_.find(host_ptr);
  if (it == allocated_list_.end()) {
    UnLock();
    MS_LOG(ERROR) << "Map buffer failed, can not found buffer :" << host_ptr;
    return nullptr;
  }

  if (it->second->map_flags) {
    UnLock();
    MS_LOG(WARNING) << "Host ptr " << host_ptr << " has mapped";
    return host_ptr;
  }
  MemBuf *mem_buf = it->second;
  void *new_host_ptr{nullptr};
  if (mem_buf->img_size.empty()) {
    cl::Buffer *buffer = static_cast<cl::Buffer *>(mem_buf->device_ptr_);
    new_host_ptr = ocl_runtime_->MapBuffer(*buffer, flags, mem_buf->size_, nullptr, sync);
  } else {
    cl::ImageFormat image_format(CL_RGBA, mem_buf->img_size[2]);
    std::vector<size_t> region{mem_buf->img_size[0], mem_buf->img_size[1], 1};
    cl::Image2D *image = static_cast<cl::Image2D *>(mem_buf->image_ptr_);
    new_host_ptr = ocl_runtime_->MapBuffer(*image, 0, CL_MAP_READ | CL_MAP_WRITE, region);
  }
  if (new_host_ptr == nullptr) {
    UnLock();
    MS_LOG(WARNING) << "Map buffer failed, can not found buffer or already mapped, dev_ptr=" << mem_buf->device_ptr_
                    << ", host_ptr=" << host_ptr;
    return nullptr;
  }

  mem_buf->map_flags = true;
  mem_buf->host_ptr_ = new_host_ptr;
  allocated_list_.erase(it);
  allocated_list_[new_host_ptr] = mem_buf;
  UnLock();
  MS_LOG(DEBUG) << "Map buffer form " << host_ptr << " to " << new_host_ptr;
  return new_host_ptr;
}

int OpenCLAllocator::UnmapBuffer(void *host_ptr, void *command_queue) {
  auto svm_capabilities = ocl_runtime_->GetSVMCapabilities();
  if (svm_capabilities) {
    if (!(svm_capabilities & CL_DEVICE_SVM_FINE_GRAIN_BUFFER)) {
      return ocl_runtime_->UnmapBuffer(host_ptr);
    }
    return RET_OK;
  }
  auto it = allocated_list_.find(host_ptr);
  if (it == allocated_list_.end()) {
    MS_LOG(ERROR) << "Map buffer failed, can not found buffer :" << host_ptr;
    return RET_ERROR;
  }
  if (it->second->map_flags) {
    it->second->map_flags = false;
    cl::Memory *mem =
      static_cast<cl::Memory *>(it->second->img_size.empty() ? it->second->device_ptr_ : it->second->image_ptr_);
    return ocl_runtime_->UnmapBuffer(*mem, it->second->host_ptr_, static_cast<cl::CommandQueue *>(command_queue));
  } else {
    MS_LOG(WARNING) << "Host ptr " << host_ptr << " do not mapped";
    return RET_OK;
  }
}

MemType OpenCLAllocator::GetMemType(void *host_ptr) {
  MemType mem_type{MemType::BUF};
  Lock();
  auto it = allocated_list_.find(host_ptr);
  if (it == allocated_list_.end()) {
    UnLock();
    MS_LOG(ERROR) << "Can not found buffer :" << host_ptr;
    return mem_type;
  }
  MemBuf *mem_buf = it->second;
  if (mem_buf->img_size.empty()) {
    mem_type = MemType::BUF;
  } else {
    mem_type = MemType::IMG;
  }
  UnLock();
  return mem_type;
}

int OpenCLAllocator::GetImageSize(void *host_ptr, std::vector<size_t> *img_size) {
  Lock();
  auto it = allocated_list_.find(host_ptr);
  if (it == allocated_list_.end()) {
    UnLock();
    MS_LOG(ERROR) << "Can not found buffer :" << host_ptr;
    return RET_OK;
  }
  MemBuf *mem_buf = it->second;
  if (!mem_buf->img_size.empty()) {
    *img_size = mem_buf->img_size;
  }
  UnLock();
  return RET_OK;
}

}  // namespace mindspore::lite::opencl
