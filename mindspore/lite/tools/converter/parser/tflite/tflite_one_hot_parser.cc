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

#include "tools/converter/parser/tflite/tflite_one_hot_parser.h"
#include <vector>
#include <memory>
#include <map>

namespace mindspore {
namespace lite {
STATUS TfliteOneHotParser::Parse(const std::unique_ptr<tflite::OperatorT> &tflite_op,
                                 const std::vector<std::unique_ptr<tflite::TensorT>> &tflite_tensors,
                                 const std::vector<std::unique_ptr<tflite::BufferT>> &tflite_model_buffer,
                                 schema::CNodeT *op, std::vector<int32_t> *tensors_id,
                                 std::vector<schema::Format> *tensors_format, std::map<int, int> *tensors_id_map) {
  MS_LOG(DEBUG) << "parse TfliteOneHotParser";
  if (op == nullptr) {
    MS_LOG(ERROR) << "op is null";
    return RET_NULL_PTR;
  }
  op->primitive = std::make_unique<schema::PrimitiveT>();
  if (op->primitive == nullptr) {
    MS_LOG(ERROR) << "op->primitive is null";
    return RET_NULL_PTR;
  }

  std::unique_ptr<schema::OneHotT> attr = std::make_unique<schema::OneHotT>();
  if (attr == nullptr) {
    MS_LOG(ERROR) << "new op failed";
    return RET_NULL_PTR;
  }

  const auto &tflite_attr = tflite_op->builtin_options.AsOneHotOptions();
  if (tflite_attr == nullptr) {
    MS_LOG(ERROR) << "get op: " << op->name << " attr failed";
    return RET_NULL_PTR;
  }
  auto axis = tflite_attr->axis;
  const auto &tensor = tflite_tensors[tflite_op->inputs[0]];
  if (tensor == nullptr) {
    MS_LOG(ERROR) << "tensor is null";
    return RET_NULL_PTR;
  }
  attr->axis = axis;

  op->primitive->value.type = schema::PrimitiveType_OneHot;
  op->primitive->value.value = attr.release();

  for (size_t i = 0; i < tflite_op->inputs.size(); i++) {
    AddOpInput(op, tensors_id, tensors_format, tensors_id_map, tflite_op->inputs[i], tensors_id->size(),
               tflite_tensors.size(), schema::Format::Format_NHWC);
  }
  AddOpOutput(op, tensors_id, tensors_format, tensors_id_map, tflite_op->outputs[0], tensors_id->size(),
              tflite_tensors.size(), schema::Format::Format_NHWC);
  return RET_OK;
}

TfliteNodeRegister g_TfliteOneHotParser("OneHot", new TfliteOneHotParser());
}  // namespace lite
}  // namespace mindspore
