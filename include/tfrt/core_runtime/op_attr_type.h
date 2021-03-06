/*
 * Copyright 2020 The TensorFlow Runtime Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//===- op_attr_type.h -------------------------------------------*- C++ -*-===//
//
// This file declares OpAttrType and helpers.
//
//===----------------------------------------------------------------------===//

#ifndef TFRT_CORE_RUNTIME_OP_ATTR_TYPE_H_
#define TFRT_CORE_RUNTIME_OP_ATTR_TYPE_H_

#include <cinttypes>
#include <utility>

#include "tfrt/support/bef_encoding.h"
#include "tfrt/support/fp16.h"

namespace tfrt {

class DenseAttr;

enum class OpAttrType : uint8_t {
  DTYPE,
  DENSE,
  F16,
#define OP_ATTR_TYPE(ENUM, CPP_TYPE) ENUM,
#include "tfrt/core_runtime/op_attr_type.def"
};

// Provide a way to get the OpAttrType for a specified C++ type at compile
// time.
template <typename T>
constexpr OpAttrType GetOpAttrType();

template <>
constexpr OpAttrType GetOpAttrType<OpAttrType>() {
  return OpAttrType::DTYPE;
}

template <>
constexpr OpAttrType GetOpAttrType<DenseAttr>() {
  return OpAttrType::DENSE;
}

template <>
constexpr OpAttrType GetOpAttrType<fp16>() {
  return OpAttrType::F16;
}

#define OP_ATTR_TYPE(ENUM, CPP_TYPE)               \
  template <>                                      \
  constexpr OpAttrType GetOpAttrType<CPP_TYPE>() { \
    return OpAttrType::ENUM;                       \
  }
#include "tfrt/core_runtime/op_attr_type.def"

// Return the size and alignment of the specified attribute type. `data` may be
// needed to decode custom scalar type to get host size and alignment.
std::pair<size_t, size_t> GetHostSizeAndAlignment(const void* data,
                                                  OpAttrType type);

// Return the name of the specified attribute type, e.g. "I32".
const char* GetNameString(OpAttrType type);

}  // namespace tfrt

#endif  // TFRT_CORE_RUNTIME_OP_ATTR_TYPE_H_
