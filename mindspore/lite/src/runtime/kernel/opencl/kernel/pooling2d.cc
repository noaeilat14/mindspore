/**
 * Copyright 2019 Huawei Technologies Co., Ltd
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

#include "src/runtime/kernel/opencl/kernel/pooling2d.h"
#include <string>
#include <set>
#include "include/errorcode.h"
#include "src/kernel_registry.h"
#include "src/runtime/kernel/opencl/utils.h"
#include "src/runtime/kernel/opencl/image_format.h"
#ifndef PROGRAM_WITH_IL
#include "src/runtime/kernel/opencl/cl/avg_pool2d.cl.inc"
#include "src/runtime/kernel/opencl/cl/max_pool2d.cl.inc"
#endif

using mindspore::kernel::KERNEL_ARCH::kGPU;
using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_INVALID_OP_NAME;
using mindspore::lite::RET_MEMORY_FAILED;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Pooling;

namespace mindspore {
namespace kernel {
int PoolingOpenCLKernel::Init() {
  std::string kernel_name;
#ifndef PROGRAM_WITH_IL
  std::string source;
  std::string program_name;
#endif
  if (parameter_->pool_mode_ == PoolMode_MaxPool) {
    kernel_name = "MaxPooling2d";
#ifndef PROGRAM_WITH_IL
    source = max_pool2d_source;
    program_name = "MaxPooling2d";
#endif
  } else if (parameter_->pool_mode_ == PoolMode_AvgPool) {
    kernel_name = "AvgPooling2d";
#ifndef PROGRAM_WITH_IL
    source = avg_pool2d_source;
    program_name = "AvgPooling2d";
#endif
  } else {
    MS_LOG(ERROR) << "Init `Pooling2d` kernel failed!";
    return RET_INVALID_OP_NAME;
  }
  enable_fp16_ = ocl_runtime_->GetFp16Enable();
#ifdef PROGRAM_WITH_IL
  kernel_ = ocl_runtime_->GetKernelFromBinary(kernel_name);
#else
  kernel_name += "_" + std::string(EnumNameFormat(op_format_));
  if (out_mem_type_ == OpenCLMemType::BUF) {
    MS_LOG(ERROR) << "buffer output not support yet.";
    return RET_ERROR;
  } else {
    kernel_name += "_IMG";
  }
  std::set<std::string> build_options;
  ocl_runtime_->LoadSource(program_name, source);
  ocl_runtime_->BuildKernel(kernel_, program_name, kernel_name, build_options);
#endif
  in_ori_format_ = in_tensors_[0]->GetFormat();
  out_ori_format_ = out_tensors_[0]->GetFormat();
  in_tensors_[0]->SetFormat(op_format_);
  out_tensors_[0]->SetFormat(op_format_);
  MS_LOG(DEBUG) << kernel_name << " Init Done!";

  return RET_OK;
}

std::vector<size_t> PoolingOpenCLKernel::InitGlobalSize() const {
  const size_t global_x = out_tensors_[0]->shape()[1];
  const size_t global_y = out_tensors_[0]->shape()[2];
  const size_t global_z = UP_DIV(out_tensors_[0]->shape()[3], C4NUM);
  std::vector<size_t> global = {global_x, global_y, global_z};
  return global;
}

int PoolingOpenCLKernel::GetImageSize(size_t idx, std::vector<size_t> *img_size) {
  size_t im_dst_x, im_dst_y;
  int n = out_tensors_[0]->shape()[0];
  int h = out_tensors_[0]->shape()[1];
  int w = out_tensors_[0]->shape()[2];
  int c = out_tensors_[0]->shape()[3];
  if (op_format_ == schema::Format::Format_NHWC4) {
    im_dst_x = w * UP_DIV(c, C4NUM);
    im_dst_y = n * h;
  } else if (op_format_ == schema::Format::Format_NC4HW4) {
    im_dst_x = w;
    im_dst_y = n * UP_DIV(c, C4NUM) * h;
  } else {
    MS_LOG(ERROR) << "not support op format:" << EnumNameFormat(op_format_);
    return RET_ERROR;
  }
  size_t img_dtype = CL_FLOAT;
  if (enable_fp16_) {
    img_dtype = CL_HALF_FLOAT;
  }
  img_size->clear();
  std::vector<size_t> vec{im_dst_x, im_dst_y, img_dtype};
  *img_size = vec;
  return RET_OK;
}

int PoolingOpenCLKernel::InitBuffer() { return RET_OK; }

int PoolingOpenCLKernel::ReSize() { return RET_OK; }

int PoolingOpenCLKernel::Run() {
  MS_LOG(DEBUG) << this->name() << " Running!";

  int slices = UP_DIV(out_tensors_[0]->shape()[3], C4NUM);
  cl_int4 input_shape = {in_tensors_[0]->shape()[1], in_tensors_[0]->shape()[2], in_tensors_[0]->shape()[3], slices};
  cl_int4 output_shape = {out_tensors_[0]->shape()[1], out_tensors_[0]->shape()[2], out_tensors_[0]->shape()[3],
                          slices};
  cl_int2 stride = {parameter_->stride_h_, parameter_->stride_w_};
  cl_int2 kernel_size = {parameter_->window_h_, parameter_->window_w_};
  cl_int2 padding = {parameter_->pad_u_, parameter_->pad_l_};

  int arg_idx = 0;
  ocl_runtime_->SetKernelArg(kernel_, arg_idx++, in_tensors_[0]->data_c());
  ocl_runtime_->SetKernelArg(kernel_, arg_idx++, out_tensors_[0]->data_c());
  ocl_runtime_->SetKernelArg(kernel_, arg_idx++, input_shape);
  ocl_runtime_->SetKernelArg(kernel_, arg_idx++, output_shape);
  ocl_runtime_->SetKernelArg(kernel_, arg_idx++, stride);
  ocl_runtime_->SetKernelArg(kernel_, arg_idx++, kernel_size);
  ocl_runtime_->SetKernelArg(kernel_, arg_idx++, padding);

  std::vector<size_t> local_size;
  std::vector<size_t> global_size = InitGlobalSize();
  int max_work_group_size = ocl_runtime_->GetKernelMaxWorkGroupSize(kernel_(), (*ocl_runtime_->Device())());
  local_size = GetCommonLocalSize(global_size, max_work_group_size);
  global_size = GetCommonGlobalSize(local_size, global_size);

  ocl_runtime_->RunKernel(kernel_, global_size, local_size, nullptr);
  return RET_OK;
}

kernel::LiteKernel *OpenCLPooling2dKernelCreator(const std::vector<lite::Tensor *> &inputs,
                                                 const std::vector<lite::Tensor *> &outputs, OpParameter *opParameter,
                                                 const lite::InnerContext *ctx, const kernel::KernelKey &desc,
                                                 const mindspore::lite::PrimitiveC *primitive) {
  auto *kernel = new (std::nothrow) PoolingOpenCLKernel(reinterpret_cast<OpParameter *>(opParameter), inputs, outputs);
  if (kernel == nullptr) {
    MS_LOG(ERROR) << "Create OpenCL Pooling kernel failed!";
    return nullptr;
  }
  auto ret = kernel->Init();
  if (RET_OK != ret) {
    MS_LOG(ERROR) << "Init OpenCL Pooling kernel failed!";
    delete kernel;
    return nullptr;
  }
  return kernel;
}

REG_KERNEL(kGPU, kNumberTypeFloat32, PrimitiveType_Pooling, OpenCLPooling2dKernelCreator)
REG_KERNEL(kGPU, kNumberTypeFloat16, PrimitiveType_Pooling, OpenCLPooling2dKernelCreator)
}  // namespace kernel
}  // namespace mindspore
