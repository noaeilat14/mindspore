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

#ifndef PREDICT_COMMON_OPTION_H_
#define PREDICT_COMMON_OPTION_H_

#include <type_traits>
#include <utility>
#include "utils/log_adapter.h"

namespace mindspore {
namespace lite {
template <typename T>
struct InnerSome {
  explicit InnerSome(const T &t) : _t(std::move(t)) {}

  T _t;
};

template <typename T>
InnerSome<typename std::decay<T>::type> Some(T &&t) {
  return InnerSome<typename std::decay<T>::type>(std::forward<T>(t));
}

struct None {};

template <typename T>
class Option {
 public:
  Option() : state(NONE) {}

  explicit Option(const T &t) : data(t), state(SOME) {}

  explicit Option(T &&t) : data(std::move(t)), state(SOME) {}

  explicit Option(const InnerSome<T> &some) : data(some._t), state(SOME) {}

  explicit Option(const None &none) : state(NONE) {}

  Option(const Option<T> &that) : state(that.state) {
    if (that.IsSome()) {
      new (&data) T(that.data);
    }
  }

  virtual ~Option() {}

  bool IsNone() const { return state == NONE; }

  bool IsSome() const { return state == SOME; }

  const T &Get() const & {
    MS_ASSERT(IsSome());
    return data;
  }

  T &Get() & {
    MS_ASSERT(IsSome());
    return data;
  }

  T &&Get() && {
    MS_ASSERT(IsSome());
    return std::move(data);
  }

  const T &&Get() const && {
    MS_ASSERT(IsSome());
    return std::move(data);
  }

  // oprerator override
  Option<T> &operator=(const Option<T> &that) {
    if (&that != this) {
      if (IsSome()) {
        data.~T();
      }
      state = that.state;
      if (that.IsSome()) {
        new (&data) T(that.data);
      }
    }

    return *this;
  }

  bool operator==(const Option<T> &that) const {
    return (IsNone() && that.IsNone()) || (IsSome() && that.IsSome() && data == that.data);
  }

  bool operator!=(const Option<T> &that) const { return !(*this == that); }

  bool operator==(const T &that) const { return IsSome() && data == that; }

  bool operator!=(const T &that) const { return !(*this == that); }

 private:
  enum State { NONE = 0, SOME = 1 };

  T data;
  State state;
};
}  // namespace lite
}  // namespace mindspore

#endif  // PREDICT_COMMON_OPTION_H_
