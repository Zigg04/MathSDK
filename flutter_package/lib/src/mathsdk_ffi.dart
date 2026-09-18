import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'package:ffi/ffi.dart';

import 'bindings_generated.dart';
import 'mathsdk_exception.dart';

extension on String {
  Pointer<Char> toNativeChar() => toNativeUtf8(allocator: calloc).cast<Char>();
}

extension on Pointer<Char> {
  /// Reads a native UTF-8 string. Throws instead of dereferencing null, so a
  /// native contract violation surfaces as a Dart exception, not a segfault.
  String readDartString() {
    if (this == nullptr) {
      throw const MathSdkException(
        status: MathSdkStatus.MATHSDK_STATUS_INTERNAL_ERROR,
        message: 'MathSDK returned OK but wrote no result string.',
      );
    }
    return cast<Utf8>().toDartString();
  }
}

/// Idiomatic Dart wrapper over the MathSDK native C ABI.
///
/// Translates every call into a Dart value or a [MathSdkException].
///
/// There is nothing to close. The native library stays loaded for the life
/// of the process, and each method frees the native memory it allocates
/// before returning, including when it throws.
///
/// ## Isolates
///
/// [MathSdk.open] returns the same instance every time within one isolate,
/// so calling it repeatedly is free. Each isolate gets its own instance —
/// do not pass a [MathSdk] across isolate boundaries, it is not sendable.
/// Call [MathSdk.open] again inside the isolate instead; the underlying
/// native library is thread-safe and shared.
///
/// ## Blocking
///
/// Every method here is synchronous and runs on the calling thread. Heavy
/// symbolic work (high-order [series], large [matrixOp], [integral]) can
/// take long enough to drop frames if called on the UI isolate. Use the
/// [MathSdkAsync] extension to run those on a background isolate.
final class MathSdk {
  MathSdk._(this._bindings);

  static MathSdk? _instance;

  /// Returns this isolate's [MathSdk], loading the native library on first
  /// call. Subsequent calls in the same isolate return the same instance.
  factory MathSdk.open() =>
      _instance ??= MathSdk._(MathSdkBindings(_openLibrary()));

  final MathSdkBindings _bindings;

  static DynamicLibrary _openLibrary() {
    if (Platform.isAndroid || Platform.isLinux) {
      return DynamicLibrary.open('libmathsdk.so');
    }
    if (Platform.isWindows) {
      return DynamicLibrary.open('mathsdk.dll');
    }
    if (Platform.isIOS || Platform.isMacOS) {
      return DynamicLibrary.process();
    }
    throw UnsupportedError('MathSDK is not available on this platform.');
  }

  /// The linked native MathSDK's version string.
  String get version => _bindings.math_sdk_version().readDartString();

  /// Parses and canonicalizes an exact expression (arithmetic, fractions,
  /// powers, roots, constants, symbolic expressions).
  String evaluate(String expression) {
    return _withOneStringResult(
      expression,
      (nativeExpression, resultOut, errorOut) =>
          _bindings.math_sdk_evaluate(nativeExpression, resultOut, errorOut),
    );
  }

  /// Same arithmetic subset as [evaluate], plus a JSON reduction trace of
  /// the steps taken to reach the result.
  (String result, String stepsJson) evaluateSteps(String expression) {
    final nativeExpression = expression.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final stepsOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_evaluate_steps(
        nativeExpression,
        resultOut,
        stepsOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return (
        resultOut.value.readDartString(),
        stepsOut.value.readDartString(),
      );
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(stepsOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeExpression);
      calloc.free(resultOut);
      calloc.free(stepsOut);
      calloc.free(errorOut);
    }
  }

  /// Replaces every occurrence of [variable] in [expression] with [value]
  /// (numeric or symbolic), then canonicalizes the result.
  String substitute({
    required String expression,
    required String variable,
    required String value,
  }) {
    final nativeExpression = expression.toNativeChar();
    final nativeVariable = variable.toNativeChar();
    final nativeValue = value.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_substitute(
        nativeExpression,
        nativeVariable,
        nativeValue,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeExpression);
      calloc.free(nativeVariable);
      calloc.free(nativeValue);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Evaluates [expression] to a [double]. No arbitrary precision — this
  /// build has no MPFR.
  double evaluateNumeric(String expression) {
    final nativeExpression = expression.toNativeChar();
    final resultOut = calloc<Double>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_evaluate_numeric(
        nativeExpression,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value;
    } finally {
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeExpression);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Derivative of [expression] with respect to [variable], of the given
  /// [order].
  ///
  /// [order] must be between 1 and 10; anything else throws a
  /// [MathSdkException] with [MathSdkStatus.MATHSDK_STATUS_INVALID_ARGUMENT].
  String derivative({
    required String expression,
    required String variable,
    int order = 1,
  }) {
    final nativeExpression = expression.toNativeChar();
    final nativeVariable = variable.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_derivative(
        nativeExpression,
        nativeVariable,
        order,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeExpression);
      calloc.free(nativeVariable);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Solves a single-variable [equation] for [variable]. Finite root sets
  /// only.
  String solve({required String equation, required String variable}) {
    final nativeEquation = equation.toNativeChar();
    final nativeVariable = variable.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_solve(
        nativeEquation,
        nativeVariable,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeEquation);
      calloc.free(nativeVariable);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Solves a linear system of [equations] for [variables]. Guarded
  /// against singular/inconsistent systems.
  String solveLinearSystem({
    required String equations,
    required String variables,
  }) {
    final nativeEquations = equations.toNativeChar();
    final nativeVariables = variables.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_solve_linear_system(
        nativeEquations,
        nativeVariables,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeEquations);
      calloc.free(nativeVariables);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Limit of [expression] as [variable] approaches [target], computed via
  /// series expansion. [direction] is `0` for two-sided, negative for
  /// left-sided, positive for right-sided.
  String limit({
    required String expression,
    required String variable,
    required String target,
    int direction = 0,
  }) {
    final nativeExpression = expression.toNativeChar();
    final nativeVariable = variable.toNativeChar();
    final nativeTarget = target.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_limit(
        nativeExpression,
        nativeVariable,
        nativeTarget,
        direction,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeExpression);
      calloc.free(nativeVariable);
      calloc.free(nativeTarget);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Indefinite integral of [expression] with respect to [variable], or
  /// the definite integral when both [lowerBound] and [upperBound] are
  /// given. Verified by differentiating the result back.
  String integral({
    required String expression,
    required String variable,
    String? lowerBound,
    String? upperBound,
  }) {
    final nativeExpression = expression.toNativeChar();
    final nativeVariable = variable.toNativeChar();
    final nativeLower = lowerBound?.toNativeChar();
    final nativeUpper = upperBound?.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_integral(
        nativeExpression,
        nativeVariable,
        nativeLower ?? nullptr,
        nativeUpper ?? nullptr,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeExpression);
      calloc.free(nativeVariable);
      if (nativeLower != null) calloc.free(nativeLower);
      if (nativeUpper != null) calloc.free(nativeUpper);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Taylor/Laurent expansion of [expression] around [aroundPoint], up to
  /// the given [order].
  ///
  /// [order] must be between 1 and 20; anything else throws a
  /// [MathSdkException] with [MathSdkStatus.MATHSDK_STATUS_INVALID_ARGUMENT].
  String series({
    required String expression,
    required String variable,
    required String aroundPoint,
    int order = 4,
  }) {
    final nativeExpression = expression.toNativeChar();
    final nativeVariable = variable.toNativeChar();
    final nativePoint = aroundPoint.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_series(
        nativeExpression,
        nativeVariable,
        nativePoint,
        order,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeExpression);
      calloc.free(nativeVariable);
      calloc.free(nativePoint);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Runs [operation] (determinant, inverse, transpose, addition,
  /// multiplication) on dense matrix [matrixA], and [matrixB] when the
  /// operation needs a second operand.
  String matrixOp({
    required String operation,
    required String matrixA,
    String? matrixB,
  }) {
    final nativeOperation = operation.toNativeChar();
    final nativeMatrixA = matrixA.toNativeChar();
    final nativeMatrixB = matrixB?.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_matrix_op(
        nativeOperation,
        nativeMatrixA,
        nativeMatrixB ?? nullptr,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeOperation);
      calloc.free(nativeMatrixA);
      if (nativeMatrixB != null) calloc.free(nativeMatrixB);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  /// Closed-form solution of the separable first-order ODE
  /// `dy/dx = fOfX(xVariable) * gOfY(yVariable)`.
  String odeSolveSeparable({
    required String fOfX,
    required String gOfY,
    required String xVariable,
    required String yVariable,
  }) {
    final nativeF = fOfX.toNativeChar();
    final nativeG = gOfY.toNativeChar();
    final nativeX = xVariable.toNativeChar();
    final nativeY = yVariable.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = _bindings.math_sdk_ode_solve_separable(
        nativeF,
        nativeG,
        nativeX,
        nativeY,
        resultOut,
        errorOut,
      );
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeF);
      calloc.free(nativeG);
      calloc.free(nativeX);
      calloc.free(nativeY);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  String _withOneStringResult(
    String input,
    MathSdkStatus Function(
      Pointer<Char> input,
      Pointer<Pointer<Char>> resultOut,
      Pointer<Pointer<Char>> errorOut,
    )
    call,
  ) {
    final nativeInput = input.toNativeChar();
    final resultOut = calloc<Pointer<Char>>();
    final errorOut = calloc<Pointer<Char>>();

    try {
      final status = call(nativeInput, resultOut, errorOut);
      _throwIfError(status, errorOut);
      return resultOut.value.readDartString();
    } finally {
      _bindings.math_sdk_free_string(resultOut.value);
      _bindings.math_sdk_free_string(errorOut.value);
      calloc.free(nativeInput);
      calloc.free(resultOut);
      calloc.free(errorOut);
    }
  }

  void _throwIfError(MathSdkStatus status, Pointer<Pointer<Char>> errorOut) {
    if (status == MathSdkStatus.MATHSDK_STATUS_OK) {
      return;
    }
    final message = errorOut.value == nullptr
        ? 'MathSDK failed with status ${status.name}.'
        : errorOut.value.readDartString();
    throw MathSdkException(status: status, message: message);
  }
}

/// Background-isolate variants of the heavier [MathSdk] operations.
///
/// Each method runs the synchronous call on a fresh isolate via
/// [Isolate.run], so the calling isolate stays responsive. Arguments and
/// results are plain [String]s, so they cross the isolate boundary freely;
/// the [MathSdk] itself never does — every closure calls [MathSdk.open]
/// inside the background isolate.
///
/// A [MathSdkException] thrown natively is rethrown here unchanged.
///
/// Spawning the isolate costs roughly 25–30 ms, which dwarfs most calls, so
/// reach for these only when the input is genuinely heavy. Measured on a
/// desktop release build, for scale:
///
/// | Call                              | Cost     |
/// | --------------------------------- | -------- |
/// | `evaluate('2 + 3 * 4')`           | 0.01 ms  |
/// | `derivative('x^3', order: 2)`     | 0.01 ms  |
/// | `evaluate` over 400 fractions     | 3.1 ms   |
/// | `solve('x^4 - 10*x^2 + 9 = 0')`   | 9.2 ms   |
/// | `series(order: 20)`               | 16.8 ms  |
///
/// A 60 fps frame has a 16.7 ms budget, so a short [evaluate] is some three
/// orders of magnitude cheaper synchronously, while a high-order [series]
/// blows the whole frame on its own. Mobile devices are slower than these
/// numbers; measure on the hardware you target.
extension MathSdkAsync on MathSdk {
  /// [MathSdk.evaluate] on a background isolate.
  Future<String> evaluateAsync(String expression) =>
      Isolate.run(() => MathSdk.open().evaluate(expression));

  /// [MathSdk.derivative] on a background isolate.
  Future<String> derivativeAsync({
    required String expression,
    required String variable,
    int order = 1,
  }) => Isolate.run(
    () => MathSdk.open().derivative(
      expression: expression,
      variable: variable,
      order: order,
    ),
  );

  /// [MathSdk.solve] on a background isolate.
  Future<String> solveAsync({
    required String equation,
    required String variable,
  }) => Isolate.run(
    () => MathSdk.open().solve(equation: equation, variable: variable),
  );

  /// [MathSdk.integral] on a background isolate.
  Future<String> integralAsync({
    required String expression,
    required String variable,
    String? lowerBound,
    String? upperBound,
  }) => Isolate.run(
    () => MathSdk.open().integral(
      expression: expression,
      variable: variable,
      lowerBound: lowerBound,
      upperBound: upperBound,
    ),
  );

  /// [MathSdk.series] on a background isolate.
  Future<String> seriesAsync({
    required String expression,
    required String variable,
    required String aroundPoint,
    int order = 4,
  }) => Isolate.run(
    () => MathSdk.open().series(
      expression: expression,
      variable: variable,
      aroundPoint: aroundPoint,
      order: order,
    ),
  );

  /// [MathSdk.matrixOp] on a background isolate.
  Future<String> matrixOpAsync({
    required String operation,
    required String matrixA,
    String? matrixB,
  }) => Isolate.run(
    () => MathSdk.open().matrixOp(
      operation: operation,
      matrixA: matrixA,
      matrixB: matrixB,
    ),
  );
}
