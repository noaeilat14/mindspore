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

#ifndef MS_OPTIMIZER_H
#define MS_OPTIMIZER_H
#include <vector>
#include "schema/inner/model_generated.h"
#include "include/errorcode.h"

namespace mindspore {
namespace lite {
using namespace schema;
template <typename T>
class Pass {
 public:
  Pass() = default;
  virtual ~Pass() = default;
  virtual STATUS Run(T *t) = 0;
};

class GraphPass : public Pass<schema::MetaGraphT> {
 public:
  GraphPass() = default;

  ~GraphPass() override = default;

  STATUS Run(schema::MetaGraphT *graph) override = 0;

  // protected:
  //  GraphDefT *graphDefT = nullptr;
};

struct GraphNode {
  GraphNode(schema::MetaGraphT *subGraph, schema::CNodeT *opDefT) : subGraph(subGraph), opDef(opDefT) {}
  ~GraphNode() = default;
  schema::MetaGraphT *subGraph = nullptr;
  schema::CNodeT *opDef = nullptr;
};

class NodePass : public Pass<GraphNode> {
 public:
  NodePass() = default;

  ~NodePass() override = default;

  STATUS Run(GraphNode *graphNode) override = 0;

  // protected:
  //  GraphNode *graphNode = nullptr;
};

class Optimizer {
 public:
  Optimizer() = default;

  virtual ~Optimizer();

  void AddPass(GraphPass *graphPass);

  void AddPass(NodePass *nodePass);

  STATUS Run(schema::MetaGraphT *graphDefT);

 private:
  std::vector<GraphPass *> graphPasses;
  std::vector<NodePass *> nodePasses;
};
}  // namespace lite
}  // namespace mindspore

#endif
