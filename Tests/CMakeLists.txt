cmake_minimum_required(VERSION 3.28)
project(runtests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_EXTENSIONS ON)

enable_testing()
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(catch2)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)

add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -Wno-pedantic>
)

add_executable(runtests TestAll.cpp)
target_link_libraries(runtests PRIVATE
  project_options
  project_warnings
  Catch2::Catch2WithMain
)

# Discover tests
include(CTest)
include(Catch)
catch_discover_tests(runtests)
