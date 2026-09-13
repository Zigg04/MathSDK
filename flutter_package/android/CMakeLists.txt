cmake_minimum_required(VERSION 3.22)

set(MATHSDK_BUILD_TESTS OFF CACHE BOOL "Disable native tests in the Flutter build" FORCE)

add_subdirectory(
  "${CMAKE_CURRENT_SOURCE_DIR}/../.."
  "${CMAKE_CURRENT_BINARY_DIR}/mathsdk"
)
