cmake_minimum_required(VERSION 3.22)

# Only used when the native sources are present — that is, when developing
# inside the MathSDK repo or consuming this package as a `path:` dependency.
# Installed from pub.dev there are no sources, and build.gradle.kts fetches a
# prebuilt .so into jniLibs instead; Gradle then skips CMake entirely.
set(MATHSDK_BUILD_TESTS OFF CACHE BOOL "Disable native tests in the Flutter build" FORCE)

add_subdirectory(
  "${CMAKE_CURRENT_SOURCE_DIR}/../.."
  "${CMAKE_CURRENT_BINARY_DIR}/mathsdk"
)
