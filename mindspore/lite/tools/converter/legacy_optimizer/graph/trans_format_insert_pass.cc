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
#include <memory>
#include <vector>
#include <utility>
#include "tools/converter/legacy_optimizer/graph/trans_format_insert_pass.h"
#include "tools/common/converter_op_utils.h"
#include "tools/common/node_util.h"
#include "utils/log_adapter.h"
#include "src/common/utils.h"

namespace mindspore {
namespace lite {
bool TransOpInsertPass::CanFusion(schema::MetaGraphT *graph, const std::unique_ptr<CNodeT> &node) {
  auto input_node_indexes = GetInputNodeIdx(*graph, *node);
  pre_type_ = schema::PrimitiveType_NONE;
  size_t has_trans_count = 0;
  auto can_fusion = true;
  for (auto input_node_index : input_node_indexes) {
    MS_ASSERT(graph->nodes.size() > input_node_index);
    auto &pre_node = graph->nodes.at(input_node_index);
    MS_ASSERT(pre_node != nullptr);
    if (pre_type_ == schema::PrimitiveType_NONE) {
      if (pre_node->primitive->value.type == schema::PrimitiveType_Nchw2Nhwc ||
          pre_node->primitive->value.type == schema::PrimitiveType_Nhwc2Nchw) {
        pre_type_ = pre_node->primitive->value.type;
        has_trans_count++;
      }
    } else {
      if (pre_node->primitive->value.type == schema::PrimitiveType_Nchw2Nhwc ||
          pre_node->primitive->value.type == schema::PrimitiveType_Nhwc2Nchw) {
        if (pre_type_ != pre_node->primitive->value.type) {
          can_fusion = false;
          break;
        } else {
          has_trans_count++;
        }
      }
    }
  }
  if (!can_fusion) {
    return false;
  }
  auto output_node_indexes = GetOutputNodeIdx(*graph, *node);
  post_type_ = schema::PrimitiveType_NONE;
  for (auto output_node_index : output_node_indexes) {
    MS_ASSERT(graph->nodes.size() > output_node_index);
    auto &post_node = graph->nodes.at(output_node_index);
    MS_ASSERT(post_node != nullptr);
    if (post_type_ == schema::PrimitiveType_NONE) {
      if (post_node->primitive->value.type == schema::PrimitiveType_Nchw2Nhwc ||
          post_node->primitive->value.type == schema::PrimitiveType_Nhwc2Nchw) {
        post_type_ = post_node->primitive->value.type;
        has_trans_count++;
      }
    } else {
      if (post_node->primitive->value.type == schema::PrimitiveType_Nchw2Nhwc ||
          post_node->primitive->value.type == schema::PrimitiveType_Nhwc2Nchw) {
        if (post_type_ != post_node->primitive->value.type) {
          can_fusion = false;
          break;
        } else {
          has_trans_count++;
        }
      }
    }
  }
  if (!can_fusion) {
    return false;
  }
  if (pre_type_ == PrimitiveType_NONE && post_type_ == PrimitiveType_NONE) {
    return false;
  }
  auto total_node_count = input_node_indexes.size() + output_node_indexes.size();
  size_t half_count = total_node_count / 2;
  if (total_node_count % 2 == 0) {
    can_fusion = has_trans_count > half_count;
  } else {
    can_fusion = has_trans_count >= half_count;
  }
  return can_fusion;
}

STATUS TransOpInsertPass::FindOutTransType() {
  pre_insert_trans_type_ = kNHWC2NCHW;
  post_insert_trans_type_ = kNHWC2NCHW;
  if (pre_type_ == PrimitiveType_NONE && post_type_ != PrimitiveType_NONE) {
    pre_insert_trans_type_ = post_type_ == schema::PrimitiveType_Nhwc2Nchw ? kNHWC2NCHW : kNCHW2NHWC;
    post_insert_trans_type_ = post_type_ == schema::PrimitiveType_Nhwc2Nchw ? kNCHW2NHWC : kNHWC2NCHW;
  } else if (pre_type_ != PrimitiveType_NONE && post_type_ == PrimitiveType_NONE) {
    pre_insert_trans_type_ = pre_type_ == schema::PrimitiveType_Nhwc2Nchw ? kNCHW2NHWC : kNHWC2NCHW;
    post_insert_trans_type_ = pre_type_ == schema::PrimitiveType_Nhwc2Nchw ? kNHWC2NCHW : kNCHW2NHWC;
  } else if (pre_type_ == PrimitiveType_NONE && post_type_ == PrimitiveType_NONE) {
    MS_ASSERT(false);
  } else {
    if (pre_type_ == post_type_) {
      MS_LOG(ERROR) << "Unknow error";
      return RET_ERROR;
    }
    pre_insert_trans_type_ = pre_type_ == schema::PrimitiveType_Nhwc2Nchw ? kNCHW2NHWC : kNHWC2NCHW;
    post_insert_trans_type_ = post_type_ == schema::PrimitiveType_Nhwc2Nchw ? kNCHW2NHWC : kNHWC2NCHW;
  }
  return RET_OK;
}

STATUS TransOpInsertPass::ChangeOpAxis(schema::MetaGraphT *graph, const std::unique_ptr<CNodeT> &node) {
  if (node == nullptr && node->primitive == nullptr) {
    MS_LOG(ERROR) << "node or primitive null";
    return RET_NULL_PTR;
  }
  auto type = node->primitive->value.type;
  if (graph->allTensors.at(node->inputIndex[0])->dims.size() != 4) {
    MS_LOG(ERROR) << "change op axis only support 4 dims";
    return RET_NOT_SUPPORT;
  }
  if (type == PrimitiveType_Concat) {
    auto origin_axis = node->primitive->value.AsConcat()->axis;
    auto axis_map = GetNc2NhAxisMap();
    node->primitive->value.AsConcat()->axis = axis_map[origin_axis];
  }
  if (type == PrimitiveType_StridedSlice) {
    auto attr = node->primitive->value.AsStridedSlice();
    auto origin_begin = attr->begin;
    attr->begin = {origin_begin[NCHW_N], origin_begin[NCHW_H], origin_begin[NCHW_W], origin_begin[NCHW_C]};
    auto origin_end = attr->end;
    attr->end = {origin_end[NCHW_N], origin_end[NCHW_H], origin_end[NCHW_W], origin_end[NCHW_C]};
    auto origin_stride = attr->stride;
    attr->stride = {origin_stride[NCHW_N], origin_stride[NCHW_H], origin_stride[NCHW_W], origin_stride[NCHW_C]};
  }
  if (type == PrimitiveType_Split) {
    auto origin_axis = node->primitive->value.AsSplit()->splitDim;
    auto axis_map = GetNc2NhAxisMap();
    node->primitive->value.AsSplit()->splitDim = axis_map[origin_axis];
  }
  return RET_OK;
}

STATUS TransOpInsertPass::Run(schema::MetaGraphT *graph) {
  MS_ASSERT(graph != nullptr);
  bool changed = true;
  int run_counts = 0;
  std::vector<CNodeT *> has_insert_nodes;
  while (changed && run_counts < 10) {
    changed = false;
    for (auto iter = graph->nodes.begin(); iter != graph->nodes.end(); iter++) {
      auto &node = *iter;
      auto type = node->primitive->value.type;
      if (IsContain(has_insert_nodes, node.get()) || !IsContain(GetInsertOpList(), type)) {
        continue;
      }
      auto node_name = node->name;
      if (!CanFusion(graph, node)) {
        continue;
      }
      auto ret = FindOutTransType();
      if (ret != RET_OK) {
        MS_LOG(ERROR) << "FindOutTransType error";
        return ret;
      }
      ret = ChangeOpAxis(graph, node);
      if (ret != RET_OK) {
        MS_LOG(ERROR) << "ChangeOpAxis error";
        return ret;
      }
      has_insert_nodes.push_back(node.get());
      STATUS status = RET_OK;
      auto input_tensor_size = (*iter)->inputIndex.size();
      for (size_t i = 0; i < input_tensor_size; i++) {
        iter = InsertFormatTransNode(graph, iter, kBefore, i, pre_insert_trans_type_, &status);
        if (status != RET_OK) {
          MS_LOG(ERROR) << "Insert" << pre_insert_trans_type_ << "before " << (*iter)->name << " failed";
          return status;
        }
      }
      auto output_tensor_size = (*iter)->outputIndex.size();
      for (size_t i = 0; i < output_tensor_size; i++) {
        iter = InsertFormatTransNode(graph, iter, kAfter, i, post_insert_trans_type_, &status);
        if (status != RET_OK) {
          MS_LOG(ERROR) << "Insert" << post_insert_trans_type_ << "Node before " << (*iter)->name << " failed";
          return status;
        }
      }
      changed = true;
    }
    run_counts++;
  }
  return RET_OK;
}
}  // namespace lite
}  // namespace mindspore
