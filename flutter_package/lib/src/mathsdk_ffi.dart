import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

import 'bindings_generated.dart';
import 'mathsdk_exception.dart';

extension on String {
  Pointer<Char> toNativeChar() =>
      toNativeUtf8(allocator: calloc).cast<Char>();
}

extension on Pointer<Char> {
  String readDartString() => cast<Utf8>().toDartString();
}

/// Idiomatic Dart wrapper over the MathSDK native C ABI.
///
/// Owns the loaded native library and translates every call into a Dart
/// value or a [MathSdkException]. Create one instance with [MathSdk.open]
/// and reuse it; each method allocates and frees its own native memory.
final class MathSdk {
  MathSdk._(this._bindings);

  /// Loads the native MathSDK library for the current platform.
  factory MathSdk.open() => MathSdk._(MathSdkBindings(_openLibrary()));

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
      (nativeExpression, resultOut, errorOut) => _bindings.math_sdk_evaluate(
        nativeExpression,
        resultOut,
        errorOut,
      ),
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
