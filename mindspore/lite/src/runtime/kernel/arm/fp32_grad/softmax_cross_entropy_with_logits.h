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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_GRAD_SOFTMAX_CROSS_ENTROPY_WITH_LOGITS_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_GRAD_SOFTMAX_CROSS_ENTROPY_WITH_LOGITS_H_

#include <vector>
#include "src/train/loss_kernel.h"
#include "ir/anf.h"
#include "nnacl/fp32_grad/softmax_grad.h"
#include "nnacl/fp32/arithmetic.h"
#include "nnacl/softmax_parameter.h"

namespace mindspore::kernel {

class SoftmaxCrossEntropyWithLogitsCPUKernel : public LossKernel {
 public:
  explicit SoftmaxCrossEntropyWithLogitsCPUKernel(OpParameter *parameter, const std::vector<lite::Tensor *> &inputs,
                                                  const std::vector<lite::Tensor *> &outputs,
                                                  const lite::InnerContext *ctx,
                                                  const mindspore::lite::PrimitiveC *primitive)
      : LossKernel(parameter, inputs, outputs, ctx, primitive), losses_(nullptr), sum_data_(nullptr) {
    param_ = reinterpret_cast<SoftmaxCrossEntropyParameter *>(parameter);
  }
  ~SoftmaxCrossEntropyWithLogitsCPUKernel() override {
    if (losses_) delete[] losses_;
    if (sum_data_) delete[] sum_data_;
  }

  void ForwardPostExecute(const float *labels, const float *logits, float *output1, float *output2) const;
  // void ForwardPostExecute(const int *labels, const float *losses, float *output) const;
  // void GradPostExecute(const int *labels, const float *losses, float* grads, float *output) const;

  int Init() override;
  int ReSize() override;
  int Run() override;

 private:
  SoftmaxCrossEntropyParameter *param_;
  SoftmaxParameter sm_params_;
  float *losses_ = nullptr;
  float *sum_data_ = nullptr;
};

}  // namespace mindspore::kernel

#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_GRAD_SOFTMAX_CROSS_ENTROPY_WITH_LOGITS_H_
