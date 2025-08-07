/*
* Copyright (c) 2025 Diago Lima
* SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once

#  if defined(__clang__)
#define RB_IS_GCC_   0
#define RB_IS_CLANG_ 1
#define RB_IS_MSVC_  0
#  elif defined(__GNUC__)
#define RB_IS_GCC_   1
#define RB_IS_CLANG_ 0
#define RB_IS_MSVC_  0
#  elif defined(_MSC_VER)
#define RB_IS_GCC_   0
#define RB_IS_CLANG_ 0
#define RB_IS_MSVC_  1
#  else
#error "unsupported compiler."
#  endif

#define NODISCARD_ [[nodiscard]]
#define UNUSED_    [[maybe_unused]]

#  if RB_IS_MSVC_
#define FORCEINLINE_ __forceinline inline
#  else
#define FORCEINLINE_ __attribute__((always_inline)) inline
#  endif
