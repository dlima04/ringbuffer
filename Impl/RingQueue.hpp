/*
* Copyright (c) 2025 Diago Lima
* SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once

#include "Common.hpp"
#include "RingBase.hpp"
#include <concepts>
#include <utility>
#include <optional>
namespace rb {

template<typename T, size_t size_>
class RingQueue : public RingBase<T, size_> {
public:
  using RingBase<T, size_>::head_;
  using RingBase<T, size_>::tail_;
  using RingBase<T, size_>::buff_;
  using RingBase<T, size_>::can_mod_opt_;
  using RingBase<T, size_>::size_mask_;

  using ValueType     = T;
  using ReferenceType = T&;
  using PointerType   = T*;

  template<typename ...Args> auto enqueue(Args&&... args) -> void {
    static_assert(std::constructible_from<T, Args...>);
    const size_t lhead = head_.load(std::memory_order::acquire);
    const size_t ltail = tail_.load(std::memory_order::acquire);

    if( ((lhead + 1) & size_mask_) == (ltail & size_mask_) ) {
      tail_.wait(ltail, std::memory_order::acquire);
    }

    buff_[ lhead & size_mask_ ] = T{std::forward<Args>(args)...};
    head_.fetch_add(1, std::memory_order::release);
    head_.notify_all();
  }

  template<typename ...Args> auto try_enqueue(Args&&... args) -> bool {
    static_assert(std::constructible_from<T, Args...>);
    const size_t lhead = head_.load(std::memory_order::acquire);
    const size_t ltail = tail_.load(std::memory_order::acquire);

    if(((lhead + 1) & size_mask_) == (ltail & size_mask_)) {
      return false;
    }

    buff_[ lhead & size_mask_ ] = T{std::forward<Args>(args)...};
    head_.fetch_add(1, std::memory_order::release);
    head_.notify_all();
    return true;
  }

  // Similarly to enqueue() and try_enqueue(), we have blocking
  // and non-blocking operations here. current() gets the current
  // element at the tail without dequeueing it.

  auto dequeue() -> ValueType {
    constexpr auto read_order  = std::memory_order::acquire;
    constexpr auto write_order = std::memory_order::release;
    const size_t lhead = head_.load(read_order);
    const size_t ltail = tail_.load(read_order);

    if((lhead & size_mask_) == (ltail & size_mask_)) {
      head_.wait(lhead, read_order);
    }

    const ValueType val = buff_[ tail_.load(read_order) & size_mask_ ];
    tail_.fetch_add(1, write_order);
    tail_.notify_all();
    return val;
  }

  auto try_dequeue() -> std::optional<ValueType> {
    const size_t lhead = head_.load(std::memory_order::acquire) & size_mask_;
    const size_t ltail = tail_.load(std::memory_order::acquire) & size_mask_;
    if(lhead == ltail) {   // the buffer is empty.
      return std::nullopt; // we can't read anything.
    }
    
    const ValueType val = buff_[ ltail ];
    tail_.fetch_add(1, std::memory_order::release);
    tail_.notify_all();
    return val;
  }

  NODISCARD_ auto try_current() const -> std::optional<ValueType> {
    const size_t lhead = head_.load(std::memory_order::acquire);
    const size_t ltail = tail_.load(std::memory_order::acquire);

    if((lhead & size_mask_) == (ltail & size_mask_)) {
      return std::nullopt;
    }

    return buff_[ ltail & size_mask_ ];
  }

  NODISCARD_ auto current() const -> ValueType {
    const size_t lhead = head_.load(std::memory_order::acquire);
    const size_t ltail = tail_.load(std::memory_order::acquire);

    if((lhead & size_mask_) == (ltail & size_mask_)) {
      head_.wait(lhead, std::memory_order::acquire);
    }

    return buff_[ ltail & size_mask_ ];
  }
  
  NODISCARD_ auto can_peek(size_t amnt) -> bool {
    const size_t lhead = head_.load(std::memory_order::acquire) & size_mask_;
    const size_t ltail = tail_.load(std::memory_order::acquire) & size_mask_;

    assert(amnt < size_); // Amount must be less than buffer size
    const size_t max_distance = lhead >= ltail
      ? lhead - ltail
      : (size_ - ltail) + lhead;
    return amnt < max_distance;
  }
  
  NODISCARD_ auto try_peek(size_t amnt) -> std::optional<ValueType> {
    const size_t lhead = head_.load(std::memory_order::acquire) & size_mask_;
    const size_t ltail = tail_.load(std::memory_order::acquire) & size_mask_;

    const size_t max_distance = lhead >= ltail
      ? lhead - ltail
      : (size_ - ltail) + lhead;
    if(amnt >= max_distance) {
      return std::nullopt;
    }

    assert(amnt < size_); // Amount must be less than buffer size
    assert(((ltail + amnt) & size_mask_) < size_); 
    return buff_[ (ltail + amnt) & size_mask_ ];
  }
  
  NODISCARD_ auto peek(size_t amnt) -> ValueType {
    const size_t ltail = tail_.load(std::memory_order::acquire) & size_mask_;
    while(!can_peek(amnt))
      head_.wait(head_.load(std::memory_order::acquire));

    return buff_[ (ltail + amnt) & size_mask_ ];
  }

  // wake any threads waiting on head_ or tail_, if any.
  auto wake_all() -> void {
    head_.notify_all();
    tail_.notify_all();
  }
  
 ~RingQueue() = default;
  RingQueue()  = default;
};

} //namespace rb