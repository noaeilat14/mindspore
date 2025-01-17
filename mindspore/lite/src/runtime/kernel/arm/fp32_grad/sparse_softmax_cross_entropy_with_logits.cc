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

#include <math.h>
#include "src/kernel_registry.h"
#include "nnacl/softmax_parameter.h"
#include "nnacl/fp32/softmax.h"
#include "src/runtime/kernel/arm/fp32_grad/sparse_softmax_cross_entropy_with_logits.h"
#include "include/errorcode.h"

using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_SoftmaxCrossEntropy;

namespace mindspore::kernel {

int SparseSoftmaxCrossEntropyWithLogitsCPUKernel::ReSize() { return RET_OK; }

int SparseSoftmaxCrossEntropyWithLogitsCPUKernel::ForwardPostExecute(const int *labels, const float *losses,
                                                                      float *output) const {
  float total_loss = 0;
  for (int i = 0; i < param->batch_size_; ++i) {
    if (labels[i] < 0) {
      MS_LOG(ERROR) << "label value must >= 0";
      return RET_ERROR;
    }
    size_t label = labels[i];
    if (label > param->number_of_classes_) {
      MS_LOG(ERROR) << "error label input!";
      return RET_ERROR;
    } else {
      total_loss -= logf(losses[i * param->number_of_classes_ + label]);
    }
  }
  output[0] = total_loss / param->batch_size_;
  return RET_OK;
}

int SparseSoftmaxCrossEntropyWithLogitsCPUKernel::GradPostExecute(const int *labels, const float *losses, float *grads,
                                                                   float *output) const {
  size_t row_start = 0;
  float total_loss = 0;
  for (int i = 0; i < param->batch_size_; ++i) {
    if (labels[i] < 0) {
      MS_LOG(ERROR) << "label value must >= 0";
      return RET_ERROR;
    }
    size_t label = labels[i];
    if (label > param->number_of_classes_) {
      MS_LOG(ERROR) << "error label input!";
      return RET_ERROR;
    } else {
      total_loss -= logf(losses[i * param->number_of_classes_ + label]);
      for (size_t j = 0; j < param->number_of_classes_; ++j) {
        size_t index = row_start + j;
        if (j == label) {
          grads[index] = (losses[index] - 1) / param->batch_size_;
        } else {
          grads[index] = losses[index] / param->batch_size_;
        }
      }
    }
    row_start += param->number_of_classes_;
  }
  output[0] = total_loss / param->batch_size_;
  return RET_OK;
}

int SparseSoftmaxCrossEntropyWithLogitsCPUKernel::Run() {
  auto ret = Prepare();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Prepare failed.";
    return ret;
  }

  auto ins = reinterpret_cast<float *>(in_tensors_.at(0)->MutableData());
  auto labels = reinterpret_cast<int *>(in_tensors_.at(1)->MutableData());
  float *out = reinterpret_cast<float *>(out_tensors_.at(0)->MutableData());
  float *grads = NULL;
  if (is_train() && out_tensors_.size() > 1) {
    grads = reinterpret_cast<float *>(out_tensors_.at(1)->MutableData());
  }
  size_t data_size = in_tensors_.at(0)->ElementsNum();
  MS_ASSERT(out != nullptr);
  MS_ASSERT(labels != nullptr);
  MS_ASSERT(ins != nullptr);
  std::fill(losses_, losses_ + data_size, 0);
  std::fill(sum_data_, sum_data_ + sm_params_.input_shape_[0], 0);
  Softmax(ins, losses_, sum_data_, &sm_params_);
  if (is_train()) {
    GradPostExecute(labels, losses_, grads, out);
  } else if (out != nullptr) {
    ForwardPostExecute(labels, losses_, out);
  }
  return RET_OK;
}

int SparseSoftmaxCrossEntropyWithLogitsCPUKernel::Init() {
  auto dims = in_tensors_[0]->shape();
  param->n_dim_ = 2;
  param->number_of_classes_ = dims[1];
  param->batch_size_ = dims[0];
  for (unsigned int i = 0; i < dims.size(); i++) param->input_shape_[i] = dims[i];
  if (2 != this->in_tensors_.size()) {
    MS_LOG(ERROR) << "softmax entropy loss should have two inputs";
    return RET_ERROR;
  }
  auto *in0 = in_tensors_.front();
  if (in0 == nullptr) {
    MS_LOG(ERROR) << "softmax etropy loss in0 have no data";
    return RET_ERROR;
  }
  size_t data_size = in_tensors_.at(0)->ElementsNum();
  losses_ = new (std::nothrow) float[data_size];
  if (losses_ == nullptr) {
    MS_LOG(ERROR) << "failed to malloc losses!";
    return RET_ERROR;
  }

  sum_data_ = new (std::nothrow) float[dims[0]];
  if (sum_data_ == nullptr) {
    MS_LOG(ERROR) << "failed to malloc sum_data_!";
    return RET_ERROR;
  }

  sm_params_.n_dim_ = 2;
  sm_params_.element_size_ = data_size;
  sm_params_.axis_ = 1;
  for (size_t i = 0; i < dims.size(); i++) sm_params_.input_shape_[i] = dims[i];

  return RET_OK;
}

kernel::LiteKernel *CpuSparseSoftmaxCrossEntropyFp32KernelCreator(
  const std::vector<lite::Tensor *> &inputs, const std::vector<lite::Tensor *> &outputs, OpParameter *opParameter,
  const lite::InnerContext *ctx, const kernel::KernelKey &desc, const mindspore::lite::PrimitiveC *primitive) {
  MS_ASSERT(opParameter != nullptr);
  MS_ASSERT(desc.type == schema::PrimitiveType_SoftmaxCrossEntropy);
  auto *kernel =
    new (std::nothrow) SparseSoftmaxCrossEntropyWithLogitsCPUKernel(opParameter, inputs, outputs, ctx, primitive);
  MS_ASSERT(kernel != nullptr);
  auto ret = kernel->Init();
  if (RET_OK != ret) {
    MS_LOG(ERROR) << "Init kernel failed, name: " << opParameter->name_ << ", type: "
                  << schema::EnumNamePrimitiveType(static_cast<schema::PrimitiveType>(opParameter->type_));
    delete kernel;
    return nullptr;
  }
  return kernel;
}

REG_KERNEL(kCPU, kNumberTypeFloat32, PrimitiveType_SoftmaxCrossEntropy, CpuSparseSoftmaxCrossEntropyFp32KernelCreator)
}  // namespace mindspore::kernel
