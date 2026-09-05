cmake_minimum_required(VERSION 3.22)

project(mathsdk_toolchain_probe LANGUAGES CXX)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

foreach(required_path
    SYMENGINE_SOURCE_DIR
    SYMENGINE_BUILD_DIR
    SYMENGINE_LIBRARY
    BOOST_ROOT)
  if(NOT DEFINED ${required_path})
    message(FATAL_ERROR "${required_path} is required")
  endif()
endforeach()

add_library(symengine STATIC IMPORTED)
set_target_properties(symengine PROPERTIES
  IMPORTED_LOCATION "${SYMENGINE_LIBRARY}"
)

add_library(mathsdk_toolchain_probe SHARED probe.cpp)
set_target_properties(mathsdk_toolchain_probe PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)
target_compile_features(mathsdk_toolchain_probe PRIVATE cxx_std_11)
target_include_directories(mathsdk_toolchain_probe PRIVATE
  "${SYMENGINE_SOURCE_DIR}"
  "${SYMENGINE_BUILD_DIR}"
  "${BOOST_ROOT}"
)
target_link_libraries(mathsdk_toolchain_probe PRIVATE symengine)

add_executable(mathsdk_toolchain_probe_runner probe_runner.cpp)
target_link_libraries(
  mathsdk_toolchain_probe_runner
  PRIVATE mathsdk_toolchain_probe
)
