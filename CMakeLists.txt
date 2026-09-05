cmake_minimum_required(VERSION 3.22)

project(MathSDK VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(MATHSDK_BUILD_TESTS "Build the MathSDK native tests" ON)
set(
  MATHSDK_SYMENGINE_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/.deps/symengine"
  CACHE PATH
  "Path to the pinned SymEngine source"
)
set(
  MATHSDK_BOOST_ROOT
  "${CMAKE_CURRENT_SOURCE_DIR}/.deps/boost_1_80_0"
  CACHE PATH
  "Path to the pinned Boost headers"
)

if(NOT EXISTS "${MATHSDK_SYMENGINE_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "SymEngine source not found: ${MATHSDK_SYMENGINE_SOURCE_DIR}")
endif()
if(NOT EXISTS "${MATHSDK_BOOST_ROOT}/boost/multiprecision/cpp_int.hpp")
  message(FATAL_ERROR "Boost headers not found: ${MATHSDK_BOOST_ROOT}")
endif()

set(INTEGER_CLASS boostmp CACHE STRING "SymEngine integer backend" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SymEngine statically" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Disable upstream tests in MathSDK builds" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "Disable SymEngine benchmarks" FORCE)
set(WITH_COTIRE OFF CACHE BOOL "Disable deprecated cotire support" FORCE)
set(WITH_SYMENGINE_THREAD_SAFE ON CACHE BOOL "Enable thread-safe SymEngine RCP" FORCE)
set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Use the pinned Boost headers" FORCE)
set(Boost_INCLUDE_DIR "${MATHSDK_BOOST_ROOT}" CACHE PATH "Boost include path" FORCE)
set(BOOST_ROOT "${MATHSDK_BOOST_ROOT}" CACHE PATH "Boost root" FORCE)

add_subdirectory(
  "${MATHSDK_SYMENGINE_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}/_deps/symengine-build"
  EXCLUDE_FROM_ALL
)

if(APPLE)
  add_library(mathsdk STATIC)
else()
  add_library(mathsdk SHARED)
endif()

target_sources(mathsdk PRIVATE native/src/math_sdk.cpp)
target_compile_features(mathsdk PRIVATE cxx_std_17)
target_compile_definitions(mathsdk PRIVATE
  MATHSDK_BUILD
  MATHSDK_VERSION="${PROJECT_VERSION}"
)
target_include_directories(mathsdk
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/native/include>"
    "$<INSTALL_INTERFACE:include>"
  PRIVATE
    "${MATHSDK_SYMENGINE_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/_deps/symengine-build"
    "${MATHSDK_BOOST_ROOT}"
)
target_link_libraries(mathsdk PRIVATE symengine)
if(UNIX AND NOT APPLE)
  target_link_options(mathsdk PRIVATE "LINKER:--exclude-libs,ALL")
endif()
set_target_properties(mathsdk PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)

include(GNUInstallDirs)
install(
  TARGETS mathsdk
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(
  FILES native/include/mathsdk/math_sdk.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mathsdk"
)

if(MATHSDK_BUILD_TESTS)
  enable_testing()
  add_executable(mathsdk_test native/tests/math_sdk_test.cpp)
  target_compile_features(mathsdk_test PRIVATE cxx_std_17)
  target_link_libraries(mathsdk_test PRIVATE mathsdk)
  add_test(NAME mathsdk_test COMMAND mathsdk_test)
endif()
