# mathsdk

A native symbolic math engine (SymEngine-backed) exposed to Flutter via
`dart:ffi`. Exact evaluation, substitution, derivatives, equation solving,
limits, integrals, series, matrices, and separable ODEs.

This is the Flutter binding for [MathSDK](https://github.com/Zigg04/MathSDK)
— the C ABI itself, its design constraints, and its evolution rules are
documented in that repository. This package adds nothing to the math the
native library supports; it only makes the ABI callable from Dart.

## Usage

```dart
import 'package:mathsdk/mathsdk.dart';

void main() {
  final sdk = MathSdk.open();

  print(sdk.evaluate('2 + 3 * 4')); // 14
  print(sdk.derivative(expression: 'x^3', variable: 'x')); // 3*x**2
  print(sdk.solve(equation: 'x^2 - 4 = 0', variable: 'x')); // ["-2","2"]
}
```

## Typed results

Several calls return JSON or a packed matrix encoding as a raw `String`,
exactly as the C ABI produced it. Typed accessors parse that for you:

```dart
sdk.solveRoots(equation: 'x^2 - 4 = 0', variable: 'x');   // ['2', '-2']
sdk.solveSystem(equations: '2*x + y = 5;x - y = 1', variables: 'x,y');
                                                          // {'x': '2', 'y': '1'}

final inverse = sdk.matrix(operation: 'inverse', matrixA: '2;2;1,2,3,4');
inverse[(row: 0, column: 0)];                             // '-2'

final (result, steps) = sdk.evaluateWithSteps('2 + 3 * 4');
steps.first.rule;                                         // 'multiply'
```

Entries stay as strings because they are exact values — `3/2` and `x + 1` do
not survive a `double`. The original `solve`, `solveLinearSystem`, `matrixOp`,
and `evaluateSteps` still return the raw strings.

Every call throws a `MathSdkException` (carrying a typed `MathSdkStatus`)
on failure instead of returning an error code — check `status` to
distinguish "not supported by this rule set" (`MATHSDK_STATUS_NOT_IMPLEMENTED`)
from a genuine input error.

## Platform support

Android only in this version, for `arm64-v8a`, `armeabi-v7a`, and `x86_64`.
iOS is not yet validated; see the MathSDK repository's README for current
platform status.

The native library is not bundled in this package. When your app builds,
Gradle downloads the prebuilt `libmathsdk.so` for each ABI from the matching
[GitHub release](https://github.com/Zigg04/MathSDK/releases) — no NDK or
CMake toolchain needed, and no multi-minute SymEngine compile. Your build
machine needs network access on the first build.

Each library is checked against the `SHA256SUMS` published with the release,
so a truncated or altered download fails the build instead of reaching your
users' devices. Verified libraries are cached under the Gradle home and
shared across projects, so `flutter clean` does not force a re-download.

If you depend on MathSDK by `path:` from a checkout of the repository, the
native sources are present and Gradle compiles them from source instead, so
local native changes are picked up.

## Instances and isolates

`MathSdk.open()` returns the same instance every time within an isolate, so
calling it repeatedly is free. There is nothing to close: the native library
stays loaded for the life of the process, and every method frees the native
memory it allocates before returning, including when it throws.

A `MathSdk` is not sendable across isolates. Call `MathSdk.open()` again
inside the isolate — the native library is thread-safe and shared.

## Running off the UI isolate

Every method on `MathSdk` is synchronous and runs on the calling thread.
Heavy symbolic work can take long enough to drop frames. The `MathSdkAsync`
extension runs the heavier operations on a background isolate:

```dart
final result = await sdk.integralAsync(expression: 'x^2', variable: 'x');
```

`evaluateAsync`, `derivativeAsync`, `solveAsync`, `integralAsync`,
`seriesAsync`, and `matrixOpAsync` are available. For cheap calls the
synchronous method is faster — spawning an isolate costs more than the math.

## Coverage and known limitations

Every function documents its exact coverage and ceiling in
`native/include/mathsdk/math_sdk.h` in the MathSDK repository — for example,
`integral` covers a table of rules (power rule, exp/log, sin/cos, linearity,
linear-argument substitution) verified by differentiating the result back,
not a general symbolic integrator. Read the header comment for the function
you're calling before assuming it covers your case; `MATHSDK_STATUS_NOT_IMPLEMENTED`
means exactly that — outside this version's supported rule set — not a bug.
