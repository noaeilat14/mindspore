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

#include "tools/converter/parser/onnx/onnx_clip_parser.h"
#include <memory>

namespace mindspore {
namespace lite {
STATUS OnnxClipParser::Parse(const onnx::GraphProto &onnx_graph, const onnx::NodeProto &onnx_node, schema::CNodeT *op) {
  MS_LOG(DEBUG) << "onnx ClipParser";
  if (op == nullptr) {
    MS_LOG(ERROR) << "op is null";
    return RET_NULL_PTR;
  }
  op->primitive = std::make_unique<schema::PrimitiveT>();
  if (op->primitive == nullptr) {
    MS_LOG(ERROR) << "op->primitive is null";
    return RET_NULL_PTR;
  }

  float min = -1, max = -1;
  for (const auto &onnx_node_attr : onnx_node.attribute()) {
    const auto &attribute_name = onnx_node_attr.name();
    if (attribute_name == "max") {
      max = onnx_node_attr.f();
    } else if (attribute_name == "min") {
      min = onnx_node_attr.f();
    }
  }
  if (min == 0 && max == 6) {
    std::unique_ptr<schema::ActivationT> attr = std::make_unique<schema::ActivationT>();
    if (attr == nullptr) {
      MS_LOG(ERROR) << "new op failed";
      return RET_NULL_PTR;
    }
    attr->type = schema::ActivationType_RELU6;

    op->primitive->value.type = schema::PrimitiveType_Activation;
    op->primitive->value.value = attr.release();
  } else {
    MS_LOG(ERROR) << "only support convert clip(0,6) to relu6, other value is not supported";
    return RET_ERROR;
  }
  return RET_OK;
}

OnnxNodeRegistrar g_onnxClipParser("Clip", new OnnxClipParser());
}  // namespace lite
}  // namespace mindspore
