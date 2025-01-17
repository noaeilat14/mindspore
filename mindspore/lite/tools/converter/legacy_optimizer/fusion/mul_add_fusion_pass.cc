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

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include "tools/converter/legacy_optimizer/fusion/mul_add_fusion_pass.h"
#include "utils/log_adapter.h"
#include "securec/include/securec.h"
#include "tools/common/graph_util.h"
#include "include/errorcode.h"
#include "schema/inner/model_generated.h"
#include "src/common/op_utils.h"

namespace mindspore {
namespace lite {
#define MUL_ADD_MATCH_PATH_LEN 2
#define ADD_OP_BIAS_INDEX 1
#define MUL_OP_BIAS_INDEX 1
#define MUL_OP_INPUT_NUM 2
#define ADD_OP_INPUT_NUM 2

STATUS MulAddFusionPass::Run(MetaGraphT *graph) { return FusionPass::Run(graph); }

STATUS MulAddFusionPass::DefinePattern() {
  auto mulOp = std::make_shared<PatternOp>();
  mulOp->id = MUL_NAME;
  mulOp->types = {schema::PrimitiveType_Mul};
  auto baOp = std::make_shared<PatternOp>();
  baOp->id = ADD_NAME;
  baOp->types = {schema::PrimitiveType_Add};
  baOp->left = mulOp;

  std::unique_ptr<FusionPattern> fusionPattern(new (std::nothrow) FusionPattern("MulAddFusion"));
  if (fusionPattern == nullptr) {
    MS_LOG(ERROR) << "new fusionPattern failed";
    return RET_ERROR;
  }
  fusionPattern->AddPatternOp(mulOp);
  fusionPattern->AddPatternOp(baOp);
  fusionPattern->Finish();

  this->patterns.emplace_back(fusionPattern.release());

  return RET_OK;
}

STATUS MulAddFusionPass::DoFusion(MetaGraphT *graph, const std::string &patternName,
                                  std::unordered_map<std::string, std::shared_ptr<Path>> &matchedPath) {
  MS_ASSERT(graph != nullptr);
  if (matchedPath.size() != MUL_ADD_MATCH_PATH_LEN) {
    MS_LOG(ERROR) << "Mul-Add-Fusion should have two NodeIndex in matchedPair";
    return RET_PARAM_INVALID;
  }

  auto mulPath = matchedPath[MUL_NAME];
  auto addPath = matchedPath[ADD_NAME];
  auto &mulNode = graph->nodes.at(mulPath->nodeIdx);
  auto &addNode = graph->nodes.at(addPath->nodeIdx);
  // can not check shape because there is now shape infer in converter
  MS_ASSERT(mulNode != nullptr);
  auto mulNodeInputIndex = mulNode->inputIndex;
  MS_ASSERT(mulNodeInputIndex.size() == MUL_OP_INPUT_NUM);
  MS_ASSERT(graph->allTensors.size() > mulNodeInputIndex.at(MUL_OP_BIAS_INDEX));
  const auto &mulNodeBiasTensor = graph->allTensors.at(mulNodeInputIndex.at(MUL_OP_BIAS_INDEX));
  MS_ASSERT(mulNodeBiasTensor != nullptr);
  if (mulNodeBiasTensor->refCount != schema::NodeType::NodeType_ValueNode || mulNodeBiasTensor->dims.size() == 4) {
    // dont fusion, return
    return RET_OK;
  }
  // add node the second tensor is not constant tensor, don't fusion
  auto addNodeInputIndex = addNode->inputIndex;
  if (addNodeInputIndex.size() != ADD_OP_INPUT_NUM) {
    MS_LOG(ERROR) << "add node input tensors number is invalid! ";  // baNode->name.c_str());
    return RET_ERROR;
  }
  MS_ASSERT(graph->allTensors.size() > addNodeInputIndex.at(ADD_OP_BIAS_INDEX));
  const auto &addNodeBiasTensor = graph->allTensors.at(addNodeInputIndex.at(ADD_OP_BIAS_INDEX));
  MS_ASSERT(addNodeBiasTensor != nullptr);
  if (addNodeBiasTensor->refCount != schema::NodeType::NodeType_ValueNode) {
    // dont fusion, return
    return RET_OK;
  }

  // convert mul and add to scale
  auto status = AddNewScaleNode(graph, mulNode, addNode.get(), addNodeInputIndex.at(ADD_OP_BIAS_INDEX));
  if (RET_OK != status) {
    MS_LOG(ERROR) << "AddFullConnectionBiasTensor failed, " << status;
    return status;
  }

  return RET_OK;
}

STATUS MulAddFusionPass::AddNewScaleNode(MetaGraphT *graph, const std::unique_ptr<CNodeT> &mulNode, CNodeT *addNode,
                                         uint32_t addBiasIndex) {
  MS_ASSERT(graph != nullptr);
  MS_ASSERT(mulNode != nullptr);
  MS_ASSERT(addNode != nullptr);
  // replace mulNode as scale
  mulNode->primitive->value.type = schema::PrimitiveType_Scale;
  std::unique_ptr<ScaleT> scaleParam(new ScaleT());
  if (scaleParam == nullptr) {
    MS_LOG(ERROR) << "new transposeParam failed";
    return RET_ERROR;
  }
  // NHWC
  int shape_size = graph->allTensors.at(addBiasIndex)->dims.size();
  scaleParam->axis = 0 - shape_size;
  mulNode->primitive->value.value = scaleParam.release();
  mulNode->inputIndex.push_back(addBiasIndex);
  if (addNode->primitive->value.AsAdd()->activationType != ActivationType_NO_ACTIVATION) {
    // repace addnode as activation
    std::unique_ptr<ActivationT> activationParam(new ActivationT());
    activationParam->type = addNode->primitive->value.AsAdd()->activationType;
    addNode->primitive->value.type = schema::PrimitiveType_Activation;
    addNode->primitive->value.value = activationParam.release();
    addNode->inputIndex.pop_back();
    return RET_OK;
  }
  // delete addnode
  auto status = IsolateOneWayNode(graph, addNode);
  if (status != RET_OK) {
    MS_LOG(ERROR) << "IsolateOneWayNode failed";
    return status;
  }
  return RET_OK;
}
}  // namespace lite
}  // namespace mindspore
