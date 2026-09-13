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

Every call throws a `MathSdkException` (carrying a typed `MathSdkStatus`)
on failure instead of returning an error code — check `status` to
distinguish "not supported by this rule set" (`MATHSDK_STATUS_NOT_IMPLEMENTED`)
from a genuine input error.

## Platform support

Android only in this version. The native library is compiled from source by
Gradle (`externalNativeBuild` + CMake) when your app builds — there is no
prebuilt binary bundled with this package. iOS is not yet validated; see the
MathSDK repository's README for current platform status.

## Coverage and known limitations

Every function documents its exact coverage and ceiling in
`native/include/mathsdk/math_sdk.h` in the MathSDK repository — for example,
`integral` covers a table of rules (power rule, exp/log, sin/cos, linearity,
linear-argument substitution) verified by differentiating the result back,
not a general symbolic integrator. Read the header comment for the function
you're calling before assuming it covers your case; `MATHSDK_STATUS_NOT_IMPLEMENTED`
means exactly that — outside this version's supported rule set — not a bug.
