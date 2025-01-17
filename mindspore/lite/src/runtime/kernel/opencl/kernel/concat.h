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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_OPENCL_KERNEL_CONCAT_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_OPENCL_KERNEL_CONCAT_H_

#include <vector>
#include "ir/anf.h"
#include "src/runtime/kernel/opencl/opencl_kernel.h"
#include "src/runtime/kernel/arm/base/concat_base.h"

namespace mindspore::kernel {

class ConcatOpenCLKernel : public OpenCLKernel {
 public:
  explicit ConcatOpenCLKernel(OpParameter *parameter, const std::vector<lite::Tensor *> &inputs,
                              const std::vector<lite::Tensor *> &outputs)
      : OpenCLKernel(parameter, inputs, outputs) {}

  ~ConcatOpenCLKernel() override{};

  int Init() override;

  int ReSize() override;

  int Run() override;

  int RunAxis0();

  int GetImageSize(size_t idx, std::vector<size_t> *img_size) override;

  int GetSumShape(std::vector<int> *sum_shape, std::vector<int> *in_shape);

 private:
  cl::Kernel kernel_;
  std::vector<int> sum_shape;
  std::vector<int> in_shape;
};

}  // namespace mindspore::kernel
#endif
