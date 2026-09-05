# MathSDK toolchain validation

This spike validates that SymEngine can be built with Boost.Multiprecision,
linked into a shared library, and called through its C wrapper. It is not the
public MathSDK API.

## Pinned inputs

| Dependency | Version | Integrity |
| --- | --- | --- |
| SymEngine | `v0.14.0` | commit `fac9314c78f2809570494017efc6603befeb4eda` |
| Boost | `1.80.0` | SHA-256 `e34756f63abe8ac34b35352743f17d061fcc825969a2dd8458264edb38781782` |
| Android NDK | `28.2.13676358` | same version used by the current Flutter SDK |
| Android API | `24` | same minimum SDK as `bernify_app` |
| CMake | `3.22.1` | Android SDK distribution |

SymEngine is configured with `INTEGER_CLASS=boostmp`, static linkage, tests
disabled for cross builds, and no GMP.

## Current result

| Target | Static library | Shared probe | Probe execution |
| --- | --- | --- | --- |
| Windows x64 / MSVC 2022 | Pass | Pass | Pass |
| Android `arm64-v8a` | Pass | Pass | Not run: no arm64 device connected |
| Android `x86_64` | Pass | Pass | Pass on `emulator-5554` |
| iOS device | Pending | Pending | Requires macOS and Xcode |
| iOS simulator | Pending | Pending | Requires macOS and Xcode |

The Android probe links the static SymEngine archive into
`libmathsdk_toolchain_probe.so`, loads it on Android, parses `2 + 3 * 4`, and
returns success only when the result is `14`.

The stripped spike libraries are approximately 6 MB per ABI. The integrated
MathSDK build now reduces that to about 1.7-1.8 MB by hiding private static
library symbols. Both need only `libm.so`, `libdl.so`, and `libc.so`; the C++
runtime is linked statically.

## Dependency setup

From the repository root in PowerShell:

```powershell
git clone --branch v0.14.0 --depth 1 `
  https://github.com/symengine/symengine.git `
  MathSDK\.deps\symengine

curl.exe -fL `
  https://archives.boost.io/release/1.80.0/source/boost_1_80_0.zip `
  -o MathSDK\.deps\boost_1_80_0.zip

Get-FileHash MathSDK\.deps\boost_1_80_0.zip -Algorithm SHA256
tar.exe -xf MathSDK\.deps\boost_1_80_0.zip -C MathSDK\.deps
```

Verify the commit and Boost hash against the pinned inputs before building.

## Windows validation

Configure SymEngine:

```powershell
$cmake = "$env:LOCALAPPDATA\Android\Sdk\cmake\3.22.1\bin\cmake.exe"

& $cmake -S MathSDK\.deps\symengine `
  -B MathSDK\.build\symengine\windows-x64 `
  -G 'Visual Studio 17 2022' -A x64 `
  -DINTEGER_CLASS=boostmp `
  -DBUILD_SHARED_LIBS=OFF `
  -DBUILD_TESTS=ON `
  -DBUILD_BENCHMARKS=OFF `
  -DWITH_COTIRE=OFF `
  -DBoost_NO_SYSTEM_PATHS=ON `
  -DBOOST_ROOT="$PWD\MathSDK\.deps\boost_1_80_0"

& $cmake --build MathSDK\.build\symengine\windows-x64 `
  --config Release --parallel 8
```

Run upstream tests:

```powershell
& "$env:LOCALAPPDATA\Android\Sdk\cmake\3.22.1\bin\ctest.exe" `
  --test-dir MathSDK\.build\symengine\windows-x64 `
  -C Release --output-on-failure
```

On MSVC, 59 of 60 upstream tests pass. `test_integer_class` expects a 64-bit
`unsigned long`, while Windows uses a 32-bit `unsigned long` under LLP64. The
library, parser, solver, C wrapper, and MathSDK probe tests pass.

Configure and run the Windows probe using the paths produced above:

```powershell
& $cmake -S MathSDK\validation `
  -B MathSDK\.build\probe\windows-x64 `
  -G 'Visual Studio 17 2022' -A x64 `
  -DSYMENGINE_SOURCE_DIR="$PWD\MathSDK\.deps\symengine" `
  -DSYMENGINE_BUILD_DIR="$PWD\MathSDK\.build\symengine\windows-x64" `
  -DSYMENGINE_LIBRARY="$PWD\MathSDK\.build\symengine\windows-x64\symengine\Release\symengine.lib" `
  -DBOOST_ROOT="$PWD\MathSDK\.deps\boost_1_80_0"

& $cmake --build MathSDK\.build\probe\windows-x64 `
  --config Release --parallel 8

& MathSDK\.build\probe\windows-x64\Release\mathsdk_toolchain_probe_runner.exe
```

## Android validation

Repeat the following configuration for `arm64-v8a` and `x86_64`, changing
`$abi` each time:

```powershell
$sdk = "$env:LOCALAPPDATA\Android\Sdk"
$cmake = "$sdk\cmake\3.22.1\bin\cmake.exe"
$ndk = "$sdk\ndk\28.2.13676358"
$abi = 'x86_64'

& $cmake -S MathSDK\.deps\symengine `
  -B "MathSDK\.build\symengine\android-$abi" `
  -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="$ndk\build\cmake\android.toolchain.cmake" `
  -DANDROID_ABI=$abi `
  -DANDROID_PLATFORM=android-24 `
  -DANDROID_STL=c++_static `
  -DCMAKE_BUILD_TYPE=Release `
  -DINTEGER_CLASS=boostmp `
  -DBUILD_SHARED_LIBS=OFF `
  -DBUILD_TESTS=OFF `
  -DBUILD_BENCHMARKS=OFF `
  -DWITH_COTIRE=OFF `
  -DBoost_NO_SYSTEM_PATHS=ON `
  -DBoost_INCLUDE_DIR="$PWD\MathSDK\.deps\boost_1_80_0"

& $cmake --build "MathSDK\.build\symengine\android-$abi" --parallel 8
```

Configure the shared probe against that static build:

```powershell
& $cmake -S MathSDK\validation `
  -B "MathSDK\.build\probe\android-$abi" `
  -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="$ndk\build\cmake\android.toolchain.cmake" `
  -DANDROID_ABI=$abi `
  -DANDROID_PLATFORM=android-24 `
  -DANDROID_STL=c++_static `
  -DCMAKE_BUILD_TYPE=Release `
  -DSYMENGINE_SOURCE_DIR="$PWD\MathSDK\.deps\symengine" `
  -DSYMENGINE_BUILD_DIR="$PWD\MathSDK\.build\symengine\android-$abi" `
  -DSYMENGINE_LIBRARY="$PWD\MathSDK\.build\symengine\android-$abi\symengine\libsymengine.a" `
  -DBOOST_ROOT="$PWD\MathSDK\.deps\boost_1_80_0"

& $cmake --build "MathSDK\.build\probe\android-$abi" --parallel 8
```

## iOS validation gate

The iOS build must run on macOS. Before shipping Dart bindings, validate both
`iphoneos` arm64 and `iphonesimulator` arm64, link the integrated MathSDK static
library into a minimal host, and execute it in the iOS simulator. Record the
Xcode, Apple Clang, CMake, and deployment target versions here with the result.
