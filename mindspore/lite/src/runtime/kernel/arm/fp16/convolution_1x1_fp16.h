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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP16_CONVOLUTION_1x1_FP16_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP16_CONVOLUTION_1x1_FP16_H_

#include <arm_neon.h>
#include <vector>
#include "src/lite_kernel.h"
#include "src/runtime/kernel/arm/fp16/convolution_base_fp16.h"
#include "nnacl/optimized_kernel.h"
#include "nnacl/matmul_parameter.h"
#include "nnacl/fp16/matmul_fp16.h"

namespace mindspore::kernel {
class Convolution1x1FP16CPUKernel : public ConvolutionBaseFP16CPUKernel {
 public:
  Convolution1x1FP16CPUKernel(OpParameter *parameter, const std::vector<lite::Tensor *> &inputs,
                              const std::vector<lite::Tensor *> &outputs, const InnerContext *ctx,
                              const mindspore::lite::PrimitiveC *primitive)
      : ConvolutionBaseFP16CPUKernel(parameter, inputs, outputs, ctx, primitive) {}
  ~Convolution1x1FP16CPUKernel() override;

  int Init() override;
  int ReSize() override;
  int Run() override;
  int RunImpl(int task_id);

 private:
  void FreeTmpBuffer();
  int InitConv1x1Param();
  int InitMatmulParam();
  int InitWeightBias();
  void Pre1x1Trans(float16_t *src_input, float16_t *src_output);

 private:
  bool pre_trans_input_ = false;
  int thread_count_ = 1;
  int thread_stride_ = 0;
  float16_t *weight_ptr_ = nullptr;
  float16_t *input_ptr_ = nullptr;
  float16_t *pack_input_ = nullptr;
  float16_t *output_ptr_ = nullptr;
  MatMulParameter *matmul_param_ = nullptr;
};
}  // namespace mindspore::kernel

#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP16_CONVOLUTION_1x1_FP16_H_
