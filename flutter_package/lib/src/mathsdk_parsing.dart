import 'dart:convert';

import 'bindings_generated.dart';
import 'mathsdk_exception.dart';
import 'mathsdk_ffi.dart';

/// A dense matrix of exact expressions, as returned by [MathSdkTyped.matrix].
///
/// Entries are kept as the strings MathSDK produced (`3/2`, `-2`, `x + 1`),
/// because they are exact values that a [double] cannot represent.
final class MathMatrix {
  /// Copies [values], so a later change to the caller's list cannot mutate
  /// this matrix.
  MathMatrix({
    required this.rowCount,
    required this.columnCount,
    required List<String> values,
  }) : _values = List.unmodifiable(values);

  /// Parses MathSDK's `rows;columns;v0,v1,...` matrix encoding.
  ///
  /// Throws a [MathSdkException] with
  /// [MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR] if [encoded] is malformed.
  factory MathMatrix.parse(String encoded) {
    final parts = encoded.split(';');
    if (parts.length != 3) {
      throw MathSdkException(
        status: MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR,
        message: 'Expected "rows;columns;values", got: $encoded',
      );
    }
    final rowCount = int.tryParse(parts[0].trim());
    final columnCount = int.tryParse(parts[1].trim());
    if (rowCount == null || columnCount == null) {
      throw MathSdkException(
        status: MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR,
        message: 'Matrix dimensions are not integers: $encoded',
      );
    }
    final values = parts[2].split(',').map((value) => value.trim()).toList();
    if (values.length != rowCount * columnCount) {
      throw MathSdkException(
        status: MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR,
        message:
            'Expected ${rowCount * columnCount} entries for a '
            '${rowCount}x$columnCount matrix, got ${values.length}.',
      );
    }
    return MathMatrix(
      rowCount: rowCount,
      columnCount: columnCount,
      values: values,
    );
  }

  final int rowCount;
  final int columnCount;
  final List<String> _values;

  /// The entry at [row] and [column], both zero-based.
  String operator [](({int row, int column}) position) {
    final (:row, :column) = position;
    RangeError.checkValidIndex(row, this, 'row', rowCount);
    RangeError.checkValidIndex(column, this, 'column', columnCount);
    return _values[row * columnCount + column];
  }

  /// The entries in row-major order. The returned list is unmodifiable.
  List<String> get values => _values;

  /// The entries grouped one list per row. The lists are unmodifiable.
  List<List<String>> get rows => List.unmodifiable([
    for (var row = 0; row < rowCount; row++)
      List<String>.unmodifiable(
        _values.sublist(row * columnCount, (row + 1) * columnCount),
      ),
  ]);

  /// Re-encodes this matrix in the form [MathSdk.matrixOp] accepts.
  String encode() => '$rowCount;$columnCount;${_values.join(',')}';

  @override
  String toString() => 'MathMatrix(${rowCount}x$columnCount, $_values)';
}

/// One step of the reduction [MathSdkTyped.evaluateWithSteps] reports.
final class MathStep {
  const MathStep({
    required this.before,
    required this.after,
    required this.rule,
  });

  /// The subexpression as it stood before this step.
  final String before;

  /// What that subexpression became.
  final String after;

  /// The name of the rule applied, as MathSDK reports it (`multiply`, `add`).
  final String rule;

  @override
  String toString() => 'MathStep($before -> $after, $rule)';
}

/// Typed accessors over the raw strings the native ABI returns.
///
/// The methods on [MathSdk] hand back exactly what the C ABI produced, which
/// for several calls is JSON or a packed matrix encoding. These variants parse
/// that at the boundary so callers do not each repeat a `jsonDecode` and hope
/// the shape matches.
///
/// Every method throws a [MathSdkException] with
/// [MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR] if the native output does not
/// have the documented shape.
extension MathSdkTyped on MathSdk {
  /// [MathSdk.solve], with the roots parsed out of the JSON array.
  List<String> solveRoots({
    required String equation,
    required String variable,
  }) {
    final raw = solve(equation: equation, variable: variable);
    return _decodeList(raw, 'solve');
  }

  /// [MathSdk.solveLinearSystem], as a variable-to-value map.
  Map<String, String> solveSystem({
    required String equations,
    required String variables,
  }) {
    final raw = solveLinearSystem(equations: equations, variables: variables);
    final decoded = _decode(raw, 'solveLinearSystem');
    if (decoded is! Map) {
      throw _parseError('solveLinearSystem', 'a JSON object', raw);
    }
    return {
      for (final entry in decoded.entries) '${entry.key}': '${entry.value}',
    };
  }

  /// [MathSdk.matrixOp], with the result parsed into a [MathMatrix].
  ///
  /// Pass a [MathMatrix] operand with [MathMatrix.encode].
  MathMatrix matrix({
    required String operation,
    required String matrixA,
    String? matrixB,
  }) {
    final raw = matrixOp(
      operation: operation,
      matrixA: matrixA,
      matrixB: matrixB,
    );
    return MathMatrix.parse(raw);
  }

  /// [MathSdk.evaluateSteps], with the trace parsed into [MathStep]s.
  (String result, List<MathStep> steps) evaluateWithSteps(String expression) {
    final (result, stepsJson) = evaluateSteps(expression);
    final decoded = _decode(stepsJson, 'evaluateSteps');
    if (decoded is! List) {
      throw _parseError('evaluateSteps', 'a JSON array', stepsJson);
    }
    final steps = <MathStep>[];
    for (final step in decoded) {
      if (step is! Map) {
        throw _parseError('evaluateSteps', 'objects in the array', stepsJson);
      }
      steps.add(
        MathStep(
          before: '${step['before']}',
          after: '${step['after']}',
          rule: '${step['rule']}',
        ),
      );
    }
    return (result, steps);
  }
}

Object? _decode(String raw, String call) {
  try {
    return jsonDecode(raw);
  } on FormatException catch (e) {
    throw MathSdkException(
      status: MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR,
      message: '$call did not return valid JSON: ${e.message}',
    );
  }
}

List<String> _decodeList(String raw, String call) {
  final decoded = _decode(raw, call);
  if (decoded is! List) {
    throw _parseError(call, 'a JSON array', raw);
  }
  return [for (final value in decoded) '$value'];
}

MathSdkException _parseError(String call, String expected, String raw) {
  return MathSdkException(
    status: MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR,
    message: '$call was expected to return $expected, got: $raw',
  );
}
