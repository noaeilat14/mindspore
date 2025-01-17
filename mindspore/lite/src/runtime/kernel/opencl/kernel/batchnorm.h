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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_OPENCL_KERNEL_BATCHNORM_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_OPENCL_KERNEL_BATCHNORM_H_

#include <vector>
#include "ir/anf.h"
#include "src/runtime/kernel/opencl/opencl_kernel.h"
#include "nnacl/fp32/batchnorm.h"

namespace mindspore::kernel {

class BatchNormOpenCLKernel : public OpenCLKernel {
 public:
  explicit BatchNormOpenCLKernel(OpParameter *parameter, const std::vector<lite::Tensor *> &inputs,
                                 const std::vector<lite::Tensor *> &outputs)
      : OpenCLKernel(parameter, inputs, outputs) {}

  ~BatchNormOpenCLKernel() override{};

  int Init() override;

  int ReSize() override;

  int Run() override;

  int GetImageSize(size_t idx, std::vector<size_t> *img_size) override;

 private:
  cl::Kernel kernel_;
};

}  // namespace mindspore::kernel
#endif
