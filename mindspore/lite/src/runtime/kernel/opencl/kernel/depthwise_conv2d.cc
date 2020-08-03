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

#include "src/runtime/kernel/opencl/kernel/depthwise_conv2d.h"
#include <string>
#include <set>
#include "src/kernel_registry.h"
#include "src/runtime/opencl/opencl_runtime.h"
#include "src/runtime/kernel/arm/fp32/convolution_depthwise.h"
#include "src/runtime/kernel/arm/opclib/pack.h"

#ifndef PROGRAM_WITH_IL

#include "src/runtime/kernel/opencl/cl/fp16/depthwise_conv2d.cl.inc"
#include "src/runtime/kernel/opencl/cl/fp32/depthwise_conv2d.cl.inc"

#endif

using mindspore::kernel::KERNEL_ARCH::kGPU;
using mindspore::lite::KernelRegistrar;
using mindspore::schema::PrimitiveType_DepthwiseConv2D;


namespace mindspore::kernel {

int DepthwiseConv2dOpenCLKernel::Init() {
  auto ocl_runtime = lite::opencl::OpenCLRuntime::GetInstance();
  std::string kernel_name = "DepthwiseConv2d";
  auto in_format = inputs_[0]->GetFormat();
  outputs_[0]->SetFormat(in_format);
  if (in_format != schema::Format_NHWC4 && in_format != schema::Format_NC4HW4) {
    MS_LOG(ERROR) << "input format(" << in_format << ") " << "format not support!";
  }
  if (mem_type_ == MEM_TYPE::BUF) {
    kernel_name += "_BUF";
  } else {
    kernel_name += "_IMG";
  }
  if (in_format == schema::Format_NC4HW4) {
    kernel_name += "_NC4HW4";
  } else if (in_format == schema::Format_NHWC4) {
    kernel_name += "_NHWC4";
  }
  auto parameter = reinterpret_cast<ConvParameter *>(opParameter);
  if (parameter->kernel_h_ == 1) {
    kernel_name += "_1x1";
  }
#ifdef PROGRAM_WITH_IL
  ocl_runtime->CreateKernelFromIL(kernel_(), kernel_name);
#else
  std::string program_name = "DepthwiseConv2d";
  std::set <std::string> build_options;
#ifdef ENABLE_FP16
  std::string source = depthwise_conv2d_source_fp16;
#else
  std::string source = depthwise_conv2d_source_fp32;
#endif
  ocl_runtime->LoadSource(program_name, source);
  ocl_runtime->BuildKernel(kernel_, program_name, kernel_name, build_options);
#endif
  this->InitBuffer();
  MS_LOG(DEBUG) << kernel_name << " Init Done!";
  return 0;
}

int DepthwiseConv2dOpenCLKernel::InitBuffer() {
  auto parameter = reinterpret_cast<ConvParameter *>(opParameter);
  auto ocl_runtime = lite::opencl::OpenCLRuntime::GetInstance();
  auto allocator = ocl_runtime->GetAllocator();

  // weight: o, h, w, i; o == group, i == 1
  auto origin_weight = reinterpret_cast<FLOAT_t *>(inputs_.at(kWeightIndex)->Data());
  int CO4 = UP_DIV(outputs_[0]->Channel(), C4NUM);
  int pack_weight_size = C4NUM * CO4 * parameter->kernel_h_ * parameter->kernel_w_;

  packed_weight_ = reinterpret_cast<FLOAT_t *>(allocator->Malloc(pack_weight_size * sizeof(FLOAT_t)));
  packed_weight_ = reinterpret_cast<FLOAT_t *>(allocator->MapBuffer(packed_weight_, CL_MAP_WRITE, nullptr, true));
  int plane = parameter->kernel_h_ * parameter->kernel_w_;
#ifdef ENABLE_FP16
  PackNCHWToNC4HW4Fp16(origin_weight, packed_weight_, 1, plane, outputs_[0]->Channel());
#else
  PackNCHWToNC4HW4Fp32(origin_weight, packed_weight_, 1, plane, outputs_[0]->Channel());
#endif

  allocator->UnmapBuffer(packed_weight_);

  // init bias
  if (inputs_.size() == kInputSize2) {
    bias_data_ = reinterpret_cast<FLOAT_t *>(allocator->Malloc(C4NUM * CO4 * sizeof(FLOAT_t)));
    bias_data_ = reinterpret_cast<FLOAT_t *>(allocator->MapBuffer(bias_data_, CL_MAP_WRITE, nullptr, true));
    size_t up_co_size = C4NUM * CO4 * sizeof(FLOAT_t);
    memset_s(bias_data_, up_co_size, 0, up_co_size);
    auto ori_bias = reinterpret_cast<FLOAT_t *>(inputs_.at(kBiasIndex)->Data());
    memcpy_s(bias_data_, outputs_[0]->Channel() * sizeof(FLOAT_t), ori_bias,
             outputs_[0]->Channel() * sizeof(FLOAT_t));
    allocator->UnmapBuffer(bias_data_);
  } else {
    MS_ASSERT(inputs_.size() == kInputSize1);
  }
  return 0;
}

int DepthwiseConv2dOpenCLKernel::ReSize() {
  return 0;
}

int DepthwiseConv2dOpenCLKernel::Run() {
  MS_LOG(DEBUG) << this->Name() << " Running!";
  auto parameter = reinterpret_cast<ConvParameter *>(opParameter);
  auto ocl_runtime = lite::opencl::OpenCLRuntime::GetInstance();
  size_t CO4 = UP_DIV(outputs_[0]->Channel(), C4NUM);
  size_t CI4 = UP_DIV(inputs_[0]->Channel(), C4NUM);
  std::vector <size_t> global = {(size_t) outputs_[0]->Width(), (size_t) outputs_[0]->Height(), CO4};
  std::vector <size_t> local = {1, 1, CO4};

  float relu_clip1 = 6.0;
  cl_int2 kernel_size = {parameter->kernel_h_, parameter->kernel_w_};
  cl_int2 stride = {parameter->stride_h_, parameter->stride_w_};
  cl_int2 padding = {-parameter->pad_h_, -parameter->pad_w_};
  cl_int2 dilation = {parameter->dilation_h_, parameter->dilation_w_};
  cl_int4 src_size = {inputs_[0]->Width(), inputs_[0]->Height(), (cl_int) CI4, inputs_[0]->Batch()};
  cl_int4 dst_size = {(cl_int) outputs_[0]->Width(), (cl_int) outputs_[0]->Height(), (cl_int) CO4,
                      (cl_int) outputs_[0]->Batch()};

  ocl_runtime->SetKernelArg(kernel_, 1, packed_weight_);
  ocl_runtime->SetKernelArg(kernel_, 2, bias_data_);
  ocl_runtime->SetKernelArg(kernel_, 3, relu_clip1);
  ocl_runtime->SetKernelArg(kernel_, 5, kernel_size);
  ocl_runtime->SetKernelArg(kernel_, 6, stride);
  ocl_runtime->SetKernelArg(kernel_, 7, padding);
  ocl_runtime->SetKernelArg(kernel_, 8, dilation);
  ocl_runtime->SetKernelArg(kernel_, 9, src_size);
  ocl_runtime->SetKernelArg(kernel_, 10, dst_size);
  if (mem_type_ == MEM_TYPE::BUF) {
    ocl_runtime->SetKernelArg(kernel_, 0, inputs_[0]->Data());
    ocl_runtime->SetKernelArg(kernel_, 4, outputs_[0]->Data());
    ocl_runtime->RunKernel(kernel_, global, local, nullptr);
  } else {
    cl::ImageFormat image_format;
    {
      image_format.image_channel_order = CL_RGBA;
      image_format.image_channel_data_type = CL_FLOAT;
    }
    cl_int in_error_code;
    size_t im_src_x, im_src_y;
    size_t im_dst_x, im_dst_y;
    if (inputs_[0]->GetFormat() == schema::Format_NHWC4) {
      im_src_x = inputs_[0]->Width() * CI4;
      im_src_y = inputs_[0]->Height();
      im_dst_x = outputs_[0]->Width() * CO4;
      im_dst_y = outputs_[0]->Height();
    } else {
      im_src_y = inputs_[0]->Height() * CI4;
      im_src_x = inputs_[0]->Width();
      im_dst_y = outputs_[0]->Height() * CO4;
      im_dst_x = outputs_[0]->Width();
    }
    cl::Image2D in_mem(*ocl_runtime->Context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_format,
                       im_src_x, im_src_y, 0, inputs_[0]->Data(), &in_error_code);
    cl_int out_error_code;
    cl::Image2D out_mem(*ocl_runtime->Context(), CL_MEM_WRITE_ONLY, image_format,
                        im_dst_x, im_dst_y, 0, nullptr, &out_error_code);
    if (in_error_code != CL_SUCCESS) {
      MS_LOG(DEBUG) << "in Image2D Failed, error=" << in_error_code;
      return 1;
    }
    if (out_error_code != CL_SUCCESS) {
      MS_LOG(DEBUG) << "out Image2D Failed, error= " << out_error_code;
      return 1;
    }
    auto origin = cl::array < cl::size_type,
    3U > {0, 0, 0};
    auto region = cl::array < cl::size_type,
    3U > {im_dst_x, im_dst_y, 1};
    ocl_runtime->SetKernelArg(kernel_, 0, in_mem);
    ocl_runtime->SetKernelArg(kernel_, 4, out_mem);

    ocl_runtime->RunKernel(kernel_, global, local, nullptr);
    ocl_runtime->GetDefaultCommandQueue()->enqueueReadImage(out_mem, CL_TRUE, origin, region, 0, 0,
                                                            outputs_[0]->Data());
  }
  return 0;
}

kernel::LiteKernel *OpenCLDepthwiseConv2dKernelCreator(const std::vector<lite::tensor::Tensor *> &inputs,
                                                       const std::vector<lite::tensor::Tensor *> &outputs,
                                                       OpParameter *opParameter, const lite::Context *ctx,
                                                       const kernel::KernelKey &desc) {
  auto *kernel = new DepthwiseConv2dOpenCLKernel(reinterpret_cast<OpParameter *>(opParameter), inputs, outputs);
  auto ret = kernel->Init();
  if (0 != ret) {
    MS_LOG(ERROR) << "Init DepthwiseConv2dOpenCLKernel failed!";
    delete kernel;
    return nullptr;
  }
  return kernel;
}

REG_KERNEL(kGPU, kNumberTypeFloat32, PrimitiveType_DepthwiseConv2D, OpenCLDepthwiseConv2dKernelCreator
)
}  // namespace mindspore::kernel

