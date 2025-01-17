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

#include "ir/graph_utils.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <stack>
#include <vector>
#include <list>
#include <string>
#include <fstream>

#include "ir/visitor.h"
#include "ir/func_graph.h"
#include "utils/label.h"
#include "utils/log_adapter.h"
#include "src/common/utils.h"

namespace mindspore {
namespace {
class DeepFirstSearcher {
 public:
  explicit DeepFirstSearcher(const IncludeFunc &include) : include_(include) {}
  ~DeepFirstSearcher() = default;

  std::vector<AnfNodePtr> Search(const AnfNodePtr &root) {
    if (root == nullptr) {
      return res_;
    }
    seen_ = NewSeenGeneration();
    Visit(root);
    return res_;
  }

  void Visit(const AnfNodePtr &node) {
    if (node == nullptr) {
      return;
    }
    if (node->seen_ == seen_) {
      return;
    }

    node->seen_ = seen_;

    auto incl = include_(node);
    if (incl == EXCLUDE) {
      return;
    }
    if (filter_ == nullptr || !filter_(node)) {
      res_.push_back(node);
    }
    if (incl == FOLLOW) {
      if (node->isa<CNode>()) {
        auto cnode = node->cast<CNodePtr>();
        auto &inputs = cnode->inputs();
        for (auto iter = inputs.rbegin(); iter != inputs.rend(); ++iter) {
          Visit(*iter);
        }
        return;
      }
    }
  }

 private:
  size_t seen_{0};
  IncludeFunc include_;
  FilterFunc filter_;
  std::vector<AnfNodePtr> res_{};
};

class DeepScopedGraphSearcher : public DeepFirstSearcher {
 public:
  explicit DeepScopedGraphSearcher(const IncludeFunc &include) : DeepFirstSearcher(include) {}
  ~DeepScopedGraphSearcher() = default;

  void Visit(const CNodePtr &cnode) { return; }

  void Visit(const ValueNodePtr &vnode) {
    if (!IsValueNode<FuncGraph>(vnode)) {
      return;
    }

    auto graph = GetValueNode<FuncGraphPtr>(vnode);
    AnfNodePtr ret = graph->get_return();
    if (ret != nullptr) {
      DeepFirstSearcher::Visit(ret);
    }
  }

  void Visit(const ParameterPtr &param) {
    if (param->func_graph() == nullptr) {
      return;
    }

    AnfNodePtr ret = param->func_graph()->get_return();
    if (ret != nullptr) {
      DeepFirstSearcher::Visit(ret);
    }
  }
};

class DeepUsedGraphSearcher : public DeepFirstSearcher {
 public:
  explicit DeepUsedGraphSearcher(const IncludeFunc &include) : DeepFirstSearcher(include) {}
  ~DeepUsedGraphSearcher() = default;

  void Visit(const CNodePtr &cnode) { return; }

  void Visit(const ValueNodePtr &vnode) { return; }
};

class DeepLinkedGraphSearcher : public DeepFirstSearcher {
 public:
  explicit DeepLinkedGraphSearcher(const IncludeFunc &include) : DeepFirstSearcher(include) {}
  ~DeepLinkedGraphSearcher() = default;

  void Visit(const CNodePtr &cnode) { return; }

  void Visit(const ValueNodePtr &) {}
};
}  // namespace

std::vector<AnfNodePtr> DeepScopedGraphSearch(const AnfNodePtr &root, const IncludeFunc &include) {
  return DeepScopedGraphSearcher(include).Search(root);
}

std::vector<AnfNodePtr> DeepUsedGraphSearch(const AnfNodePtr &root, const IncludeFunc &include) {
  return DeepUsedGraphSearcher(include).Search(root);
}

std::vector<AnfNodePtr> DeepLinkedGraphSearch(const AnfNodePtr &root, const IncludeFunc &include) {
  return DeepLinkedGraphSearcher(include).Search(root);
}
}  // namespace mindspore
