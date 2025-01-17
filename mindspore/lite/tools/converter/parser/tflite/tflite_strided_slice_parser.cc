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

#include "tools/converter/parser/tflite/tflite_strided_slice_parser.h"
#include <vector>
#include <memory>
#include <map>

namespace mindspore {
namespace lite {
STATUS TfliteStridedSliceParser::Parse(const std::unique_ptr<tflite::OperatorT> &tflite_op,
                                       const std::vector<std::unique_ptr<tflite::TensorT>> &tflite_tensors,
                                       const std::vector<std::unique_ptr<tflite::BufferT>> &tflite_model_buffer,
                                       schema::CNodeT *op, std::vector<int32_t> *tensors_id,
                                       std::vector<schema::Format> *tensors_format,
                                       std::map<int, int> *tensors_id_map) {
  MS_LOG(DEBUG) << "parse TfliteStridedSliceParser";
  if (op == nullptr) {
    MS_LOG(ERROR) << "op is null";
    return RET_NULL_PTR;
  }
  op->primitive = std::make_unique<schema::PrimitiveT>();
  if (op->primitive == nullptr) {
    MS_LOG(ERROR) << "op->primitive is null";
    return RET_NULL_PTR;
  }

  std::unique_ptr<schema::StridedSliceT> attr = std::make_unique<schema::StridedSliceT>();
  if (attr == nullptr) {
    MS_LOG(ERROR) << "new op failed";
    return RET_NULL_PTR;
  }

  const auto &tflite_attr = tflite_op->builtin_options.AsStridedSliceOptions();
  if (tflite_attr == nullptr) {
    MS_LOG(ERROR) << "get op: %s attr failed", op->name.c_str();
    return RET_NULL_PTR;
  }
  attr->beginMask = tflite_attr->begin_mask;
  attr->endMask = tflite_attr->end_mask;
  attr->ellipsisMask = tflite_attr->ellipsis_mask;
  attr->newAxisMask = tflite_attr->new_axis_mask;
  attr->shrinkAxisMask = tflite_attr->shrink_axis_mask;

  if (GetTfliteData(tflite_op->inputs[1], tflite_tensors, tflite_model_buffer, attr->begin)) {
    MS_LOG(ERROR) << "stridedSlice -> begin get failed";
    return RET_ERROR;
  }
  if (GetTfliteData(tflite_op->inputs[2], tflite_tensors, tflite_model_buffer, attr->end)) {
    MS_LOG(ERROR) << "stridedSlice -> end get failed";
    return RET_ERROR;
  }
  if (GetTfliteData(tflite_op->inputs[3], tflite_tensors, tflite_model_buffer, attr->stride)) {
    MS_LOG(ERROR) << "stridedSlice -> stride get failed";
    return RET_ERROR;
  }
  attr->isScale.assign(tflite_tensors[tflite_op->inputs[0]]->shape.begin(),
                       tflite_tensors[tflite_op->inputs[0]]->shape.end());

  op->primitive->value.type = schema::PrimitiveType_StridedSlice;
  op->primitive->value.value = attr.release();

  AddOpInput(op, tensors_id, tensors_format, tensors_id_map, tflite_op->inputs[0], tensors_id->size(),
             tflite_tensors.size(), schema::Format::Format_NHWC);
  AddOpOutput(op, tensors_id, tensors_format, tensors_id_map, tflite_op->outputs[0], tensors_id->size(),
              tflite_tensors.size(), schema::Format::Format_NHWC);
  return RET_OK;
}

TfliteNodeRegister g_tfliteStridedSliceParser("StridedSlice", new TfliteStridedSliceParser());
}  // namespace lite
}  // namespace mindspore
