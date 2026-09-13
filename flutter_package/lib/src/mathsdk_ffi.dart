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

final class MathSdk {
  MathSdk._(this._bindings);

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

  String get version => _bindings.math_sdk_version().readDartString();

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
