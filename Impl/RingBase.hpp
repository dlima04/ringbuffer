/*
* Copyright (c) 2025 Diago Lima
* SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once

#include "Common.hpp"
#include <atomic>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <cassert>
namespace rb {

template<typename T, size_t size_>
class RingBase {
public:
  constexpr static bool can_mod_opt_ = (size_ & (size_ - 1)) == 0;
  constexpr static size_t size_mask_ = size_ - 1;

  static_assert(std::is_trivially_destructible_v<T>);
  static_assert(can_mod_opt_, "size must be a power of 2!");
  static_assert(size_ > 1, "Size must be greater than 1!");

  NODISCARD_ auto is_full()  const -> bool {
    constexpr auto order  = std::memory_order::acquire;
    const size_t lhead    = (head_.load(order) + 1) & size_mask_;
    const size_t ltail    = tail_.load(order) & size_mask_;
    return lhead == ltail;
  }

  NODISCARD_ auto is_empty() const -> bool {
    constexpr auto order  = std::memory_order::acquire;
    const size_t lhead    = head_.load(order) & size_mask_;
    const size_t ltail    = tail_.load(order) & size_mask_;
    return lhead == ltail;
  }

  NODISCARD_ FORCEINLINE_ auto* data(this auto&& self) {
    return std::forward<decltype(self)>(self).buff_;
  }

 ~RingBase() = default;
  RingBase() = default;
protected:
  alignas(64) T buff_[ size_ ]{};
  alignas(64) std::atomic<size_t> head_{ 0 };
  alignas(64) std::atomic<size_t> tail_{ 0 };
};

} //namespace rb