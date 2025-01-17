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
#ifndef MINDSPORE_LITE_SRC_BACKEND_ARM_FP32_SPACE_TO_DEPTH_H_
#define MINDSPORE_LITE_SRC_BACKEND_ARM_FP32_SPACE_TO_DEPTH_H_

#include <vector>
#include "src/lite_kernel.h"
#include "ir/anf.h"

namespace mindspore::kernel {
class SpaceToDepthCPUKernel : public LiteKernel {
 public:
  SpaceToDepthCPUKernel(OpParameter *parameter, const std::vector<lite::Tensor *> &inputs,
                        const std::vector<lite::Tensor *> &outputs, const lite::InnerContext *ctx,
                        const mindspore::lite::PrimitiveC *primitive)
      : LiteKernel(parameter, inputs, outputs, ctx, primitive) {}
  ~SpaceToDepthCPUKernel() = default;

  int SpaceToDepth(int task_id);
  int Init() override;
  int ReSize() override;
  int Run() override;

 private:
  int thread_h_stride_;
  int thread_h_num_;
  int num_unit_;
  float *input_ptr_;
  float *output_ptr_;
};
}  // namespace mindspore::kernel

#endif  // MINDSPORE_LITE_SRC_BACKEND_ARM_FP32_SPACE_TO_DEPTH_H_
