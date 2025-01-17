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

#ifndef MINDSPORE_LITE_TOOLS_CONVERTER_PARSER_TFLITE_ACTIVATION_PARSER_H
#define MINDSPORE_LITE_TOOLS_CONVERTER_PARSER_TFLITE_ACTIVATION_PARSER_H

#include <vector>
#include <memory>
#include <map>
#include "tools/converter/parser/tflite/tflite_node_parser.h"
#include "tools/converter/parser/tflite/tflite_node_parser_registry.h"

namespace mindspore {
namespace lite {
class TfliteActivationParser : public TfliteNodeParser {
 public:
  TfliteActivationParser() : TfliteNodeParser("node_name") {}

  STATUS Parse(const std::unique_ptr<tflite::OperatorT> &tflite_op,
               const std::vector<std::unique_ptr<tflite::TensorT>> &tflite_tensors,
               const std::vector<std::unique_ptr<tflite::BufferT>> &tflite_model_buffer, schema::CNodeT *op,
               std::vector<int32_t> *tensors_id, std::vector<schema::Format> *tensors_format,
               std::map<int, int> *tensors_id_map) override;
};

class TfliteReluParser : public TfliteActivationParser {
 public:
  TfliteReluParser() : TfliteActivationParser() {}
};

class TfliteRelu6Parser : public TfliteActivationParser {
 public:
  TfliteRelu6Parser() : TfliteActivationParser() {}
};

class TfliteTanhParser : public TfliteActivationParser {
 public:
  TfliteTanhParser() : TfliteActivationParser() {}
};

class TfliteLogisticParser : public TfliteActivationParser {
 public:
  TfliteLogisticParser() : TfliteActivationParser() {}
};

class TfliteHardSwishParser : public TfliteActivationParser {
 public:
  TfliteHardSwishParser() : TfliteActivationParser() {}
};

class TfliteLeakyReluParser : public TfliteActivationParser {
 public:
  TfliteLeakyReluParser() : TfliteActivationParser() {}
};

}  // namespace lite
}  // namespace mindspore

#endif  // MINDSPORE_LITE_TOOLS_CONVERTER_PARSER_TFLITE_ACTIVATION_PARSER_H
