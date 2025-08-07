/*
* Copyright (c) 2025 Diago Lima
* SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once

#include "Common.hpp"
#include "RingBase.hpp"
#include <optional>
#include <utility>
#include <concepts>
namespace rb {

template<typename T, size_t size_>
class RingBuffer : public RingBase<T, size_>{
public:
  using ValueType     = T;
  using ReferenceType = T&;
  using PointerType   = T*;

  using RingBase<T, size_>::head_;
  using RingBase<T, size_>::tail_;
  using RingBase<T, size_>::buff_;
  using RingBase<T, size_>::can_mod_opt_;
  using RingBase<T, size_>::size_mask_;

  /// write() and overwrite() will attempt to construct
  /// an object of type T directly at the head index using the
  /// parameter pack Args.

  template<typename ...Args>
  auto write(Args&&... args) -> bool {
    static_assert(std::constructible_from<T, Args...>);
    if(this->is_full())
      return false;

    buff_[ head_.load() & size_mask_ ] = T(std::forward<Args>(args)...);
    head_.fetch_add(1, std::memory_order::release);
    return true;
  }

  template<typename ...Args>
  auto overwrite(Args&&... args) -> void {
    static_assert(std::constructible_from<T, Args...>);
    if(this->is_full())
      tail_.fetch_add(1, std::memory_order::release);

    buff_[ head_.load() & size_mask_ ] = T(std::forward<Args>(args)...);
    head_.fetch_add(1, std::memory_order::release);
  }

  /// For reading values from the ringbuffer.
  /// note that current() and try_current retrieve the value at
  /// tail_ without incrementing it.

  auto read() -> std::optional<ValueType> {
    const size_t lhead = head_.load(std::memory_order::acquire) & size_mask_;
    const size_t ltail = tail_.load(std::memory_order::acquire) & size_mask_;

    if(lhead == ltail)      /// buffer is empty.
      return std::nullopt;  /// we can't read anything.

    const ValueType val = buff_[ ltail ];
    tail_.fetch_add(1, std::memory_order::release);
    return val;
  }

  NODISCARD_ auto try_current() const -> std::optional<ValueType> {
    const size_t lhead = head_.load(std::memory_order::acquire);
    const size_t ltail = tail_.load(std::memory_order::acquire);

    if((lhead & size_mask_) == (ltail & size_mask_))
      return std::nullopt;

    return buff_[ ltail & size_mask_ ];
  }

  NODISCARD_ auto current() const -> ValueType {
    const size_t lhead = head_.load(std::memory_order::acquire);
    const size_t ltail = tail_.load(std::memory_order::acquire);

    if((lhead & size_mask_) == (ltail & size_mask_))
      head_.wait(lhead, std::memory_order::acquire);

    return buff_[ ltail & size_mask_ ];
  }

 ~RingBuffer() = default;
  RingBuffer()  = default;
};

} //namespace rb
