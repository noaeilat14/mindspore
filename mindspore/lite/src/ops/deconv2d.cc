/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "src/ops/deconv2d.h"
#include <memory>
#include <string>

namespace mindspore {
namespace lite {
#ifdef PRIMITIVE_WRITEABLE
int DeConv2D::GetFormat() const { return this->primitive_->value.AsDeConv2D()->format; }
int DeConv2D::GetGroup() const { return this->primitive_->value.AsDeConv2D()->group; }
int DeConv2D::GetChannelIn() const { return this->primitive_->value.AsDeConv2D()->channelIn; }
int DeConv2D::GetChannelOut() const { return this->primitive_->value.AsDeConv2D()->channelOut; }
int DeConv2D::GetKernelW() const { return this->primitive_->value.AsDeConv2D()->kernelW; }
int DeConv2D::GetKernelH() const { return this->primitive_->value.AsDeConv2D()->kernelH; }
int DeConv2D::GetStrideW() const { return this->primitive_->value.AsDeConv2D()->strideW; }
int DeConv2D::GetStrideH() const { return this->primitive_->value.AsDeConv2D()->strideH; }
int DeConv2D::GetPadMode() const { return this->primitive_->value.AsDeConv2D()->padMode; }
int DeConv2D::GetPadUp() const { return this->primitive_->value.AsDeConv2D()->padUp; }
int DeConv2D::GetPadDown() const { return this->primitive_->value.AsDeConv2D()->padDown; }
int DeConv2D::GetPadLeft() const { return this->primitive_->value.AsDeConv2D()->padLeft; }
int DeConv2D::GetPadRight() const { return this->primitive_->value.AsDeConv2D()->padRight; }
int DeConv2D::GetDilateW() const { return this->primitive_->value.AsDeConv2D()->dilateW; }
int DeConv2D::GetDilateH() const { return this->primitive_->value.AsDeConv2D()->dilateH; }
bool DeConv2D::GetHasBias() const { return this->primitive_->value.AsDeConv2D()->hasBias; }
int DeConv2D::GetActivationType() const { return this->primitive_->value.AsDeConv2D()->activationType; }

void DeConv2D::SetFormat(int format) { this->primitive_->value.AsDeConv2D()->format = (schema::Format)format; }
void DeConv2D::SetGroup(int group) { this->primitive_->value.AsDeConv2D()->group = group; }
void DeConv2D::SetChannelIn(int channel_in) { this->primitive_->value.AsDeConv2D()->channelIn = channel_in; }
void DeConv2D::SetChannelOut(int channel_out) { this->primitive_->value.AsDeConv2D()->channelOut = channel_out; }
void DeConv2D::SetKernelW(int kernel_w) { this->primitive_->value.AsDeConv2D()->kernelW = kernel_w; }
void DeConv2D::SetKernelH(int kernel_h) { this->primitive_->value.AsDeConv2D()->kernelH = kernel_h; }
void DeConv2D::SetStrideW(int stride_w) { this->primitive_->value.AsDeConv2D()->strideW = stride_w; }
void DeConv2D::SetStrideH(int stride_h) { this->primitive_->value.AsDeConv2D()->strideH = stride_h; }
void DeConv2D::SetPadMode(int pad_mode) { this->primitive_->value.AsDeConv2D()->padMode = (schema::PadMode)pad_mode; }
void DeConv2D::SetPadUp(int pad_up) { this->primitive_->value.AsDeConv2D()->padUp = pad_up; }
void DeConv2D::SetPadDown(int pad_down) { this->primitive_->value.AsDeConv2D()->padDown = pad_down; }
void DeConv2D::SetPadLeft(int pad_left) { this->primitive_->value.AsDeConv2D()->padLeft = pad_left; }
void DeConv2D::SetPadRight(int pad_right) { this->primitive_->value.AsDeConv2D()->padRight = pad_right; }
void DeConv2D::SetDilateW(int dilate_w) { this->primitive_->value.AsDeConv2D()->dilateW = dilate_w; }
void DeConv2D::SetDilateH(int dilate_h) { this->primitive_->value.AsDeConv2D()->dilateH = dilate_h; }
void DeConv2D::SetHasBias(bool has_bias) { this->primitive_->value.AsDeConv2D()->hasBias = has_bias; }
void DeConv2D::SetActivationType(int activation_type) {
  this->primitive_->value.AsDeConv2D()->activationType = (schema::ActivationType)activation_type;
}
void DeConv2D::PopulaterDeConv2DSingleGroup(const Primitive &prim, schema::PrimitiveT *primitive, const int &group) {
  auto attr = std::make_unique<schema::DeConv2DT>();
  attr->group = group;
  auto format = GetValue<std::string>(prim.GetAttr("data_format"));
  if (format == "NCHW") {
    attr->format = schema::Format_NCHW;
  } else if (format == "NHWC") {
    attr->format = schema::Format_NHWC;
  } else {
    attr->format = schema::Format_NUM_OF_FORMAT;
  }
  auto pad_list = GetValue<std::vector<int>>(prim.GetAttr("pad_list"));
  attr->padUp = pad_list[0];
  attr->padDown = pad_list[1];
  attr->padLeft = pad_list[2];
  attr->padRight = pad_list[3];

  auto dilation = GetValue<std::vector<int>>(prim.GetAttr("dilation"));
  attr->dilateH = dilation[0];
  attr->dilateW = dilation[1];

  auto kernel_size = GetValue<std::vector<int>>(prim.GetAttr("kernel_size"));
  attr->kernelH = kernel_size[0];
  attr->kernelW = kernel_size[1];

  auto stride = GetValue<std::vector<int>>(prim.GetAttr("stride"));
  attr->strideH = stride[0];
  attr->strideW = stride[1];

  attr->channelOut = GetValue<int>(prim.GetAttr("out_channel"));

  auto pad_mode = GetValue<std::string>(prim.GetAttr("pad_mode"));
  if (pad_mode == "valid" || pad_mode == "VALID") {
    attr->padMode = schema::PadMode_VALID;
  } else if (pad_mode == "same" || pad_mode == "SAME") {
    attr->padMode = schema::PadMode_SAME_UPPER;
  } else {
    attr->padMode = schema::PadMode_NOTSET;
  }

  if (prim.GetAttr("activation_name") != nullptr) {
    std::string activate_name = GetValue<std::string>(prim.GetAttr("activation_name"));
    attr->activationType = kActivationTypeMap[activate_name];
  } else {
    attr->activationType = schema::ActivationType_NO_ACTIVATION;
  }

  primitive->value.type = schema::PrimitiveType_DeConv2D;
  primitive->value.value = attr.release();
}

int DeConv2D::UnPackAttr(const Primitive &prim, const std::vector<AnfNodePtr> &inputs) {
  if (this->primitive_ == nullptr) {
    this->primitive_ = new (std::nothrow) schema::PrimitiveT;
    if (this->primitive_ == nullptr) {
      MS_LOG(ERROR) << "new primitiveT failed";
      return RET_ERROR;
    }
    this->primitive_->value.type = schema::PrimitiveType_DeConv2D;
  }
  if (this->primitive_->value.type != schema::PrimitiveType_DeConv2D) {
    MS_LOG(ERROR) << "primitive_ type is error:" << this->primitive_->value.type;
    return RET_ERROR;
  }
  int group = GetValue<int>(prim.GetAttr("group"));
  if (group == 1) {
    PopulaterDeConv2DSingleGroup(prim, this->primitive_, group);
  }

  if (GetQuantType() == schema::QuantType_AwareTraining) {
    std::vector<std::vector<schema::QuantParamT>> vecInputQuantParam;
    std::vector<std::vector<schema::QuantParamT>> vecOutputQuantParam;
    PopulaterQuantParam(prim, &vecInputQuantParam, &vecOutputQuantParam, inputs);
    SetInputQuantParam(vecInputQuantParam);
    SetOutputQuantParam(vecOutputQuantParam);
  }
  return RET_OK;
}
#else
int DeConv2D::UnPackToFlatBuilder(const schema::Primitive *primitive, flatbuffers::FlatBufferBuilder *fbb) {
  MS_ASSERT(nullptr != primitive);
  MS_ASSERT(nullptr != fbb);
  auto attr = primitive->value_as_DeConv2D();
  if (attr == nullptr) {
    MS_LOG(ERROR) << "value_as_DeConv2D return nullptr";
    return RET_ERROR;
  }
  auto val_offset = schema::CreateDeConv2D(
    *fbb, attr->format(), attr->group(), attr->channelIn(), attr->channelOut(), attr->kernelW(), attr->kernelH(),
    attr->strideW(), attr->strideH(), attr->padMode(), attr->padUp(), attr->padDown(), attr->padLeft(),
    attr->padRight(), attr->dilateW(), attr->dilateH(), attr->hasBias(), attr->activationType());
  auto prim_offset = schema::CreatePrimitive(*fbb, schema::PrimitiveType_DeConv2D, val_offset.o);
  fbb->Finish(prim_offset);
  return RET_OK;
}
int DeConv2D::GetFormat() const { return this->primitive_->value_as_DeConv2D()->format(); }
int DeConv2D::GetGroup() const { return this->primitive_->value_as_DeConv2D()->group(); }
int DeConv2D::GetChannelIn() const { return this->primitive_->value_as_DeConv2D()->channelIn(); }
int DeConv2D::GetChannelOut() const { return this->primitive_->value_as_DeConv2D()->channelOut(); }
int DeConv2D::GetKernelW() const { return this->primitive_->value_as_DeConv2D()->kernelW(); }
int DeConv2D::GetKernelH() const { return this->primitive_->value_as_DeConv2D()->kernelH(); }
int DeConv2D::GetStrideW() const { return this->primitive_->value_as_DeConv2D()->strideW(); }
int DeConv2D::GetStrideH() const { return this->primitive_->value_as_DeConv2D()->strideH(); }
int DeConv2D::GetPadMode() const { return this->primitive_->value_as_DeConv2D()->padMode(); }
int DeConv2D::GetPadUp() const { return this->primitive_->value_as_DeConv2D()->padUp(); }
int DeConv2D::GetPadDown() const { return this->primitive_->value_as_DeConv2D()->padDown(); }
int DeConv2D::GetPadLeft() const { return this->primitive_->value_as_DeConv2D()->padLeft(); }
int DeConv2D::GetPadRight() const { return this->primitive_->value_as_DeConv2D()->padRight(); }
int DeConv2D::GetDilateW() const { return this->primitive_->value_as_DeConv2D()->dilateW(); }
int DeConv2D::GetDilateH() const { return this->primitive_->value_as_DeConv2D()->dilateH(); }
bool DeConv2D::GetHasBias() const { return this->primitive_->value_as_DeConv2D()->hasBias(); }
int DeConv2D::GetActivationType() const { return this->primitive_->value_as_DeConv2D()->activationType(); }

#endif
int DeConv2D::InferShape(std::vector<lite::Tensor *> inputs_, std::vector<lite::Tensor *> outputs_) {
  MS_ASSERT(this->primitive_ != nullptr);
  auto input = inputs_.front();
  MS_ASSERT(input != nullptr);
  auto weight = inputs_.at(1);
  MS_ASSERT(weight != nullptr);
  auto output = outputs_.front();
  MS_ASSERT(output != nullptr);
  output->SetFormat(input->GetFormat());
  output->set_data_type(input->data_type());
  if (!GetInferFlag()) {
    return RET_OK;
  }
  int32_t input_h = input->Height();
  int32_t input_w = input->Width();

  int32_t output_n = input->Batch();
  int32_t output_h = 0;
  int32_t output_w = 0;
  int32_t output_c = weight->Channel();

  int kernel_w = GetKernelW();
  int kernel_h = GetKernelH();
  int stride_w = GetStrideW();
  int stride_h = GetStrideH();
  int dilate_w = GetDilateW();
  int dilate_h = GetDilateH();
  pad_l_ = GetPadLeft();
  pad_u_ = GetPadUp();
  pad_d_ = GetPadDown();
  pad_r_ = GetPadRight();
  auto pad_mode = (schema::PadMode)GetPadMode();
  if (pad_mode == schema::PadMode_CAFFE || pad_mode == schema::PadMode_NOTSET) {
    output_h = (input_h - 1) * stride_h + ((kernel_h - 1) * dilate_h + 1) - pad_u_ - pad_d_;
    output_w = (input_w - 1) * stride_w + ((kernel_w - 1) * dilate_w + 1) - pad_l_ - pad_r_;
  } else if (pad_mode == schema::PadMode_SAME_UPPER) {
    output_h = input_h * stride_h;
    output_w = input_w * stride_w;
  } else if (pad_mode == schema::PadMode_VALID) {
    output_h = (input_h - 1) * stride_h + kernel_h;
    output_w = (input_w - 1) * stride_w + kernel_w;
  } else {
    MS_LOG(ERROR) << "unsupported pad mode for deconv";
    return RET_ERROR;
  }
  std::vector<int> out_shape = {output_n, output_h, output_w, output_c};
  output->set_shape(out_shape);

  if (pad_mode == schema::PadMode_SAME_UPPER) {
    pad_u_ = ((input_h - 1) * stride_h + (kernel_h - 1) * dilate_h + 1 - output_h) / 2;
    pad_l_ = ((input_w - 1) * stride_w + (kernel_w - 1) * dilate_w + 1 - output_w) / 2;
  } else if (pad_mode == schema::PadMode_VALID) {
    pad_u_ = 0;
    pad_l_ = 0;
  } else if (pad_mode == schema::PadMode_CAFFE || pad_mode == schema::PadMode_NOTSET) {
  } else {
    MS_LOG(ERROR) << "unsupported pad mode for deconv";
    return RET_ERROR;
  }

  return 0;
}
}  // namespace lite
}  // namespace mindspore
