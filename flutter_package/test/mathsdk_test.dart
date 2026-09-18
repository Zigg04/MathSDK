import 'dart:io';

import 'package:mathsdk/mathsdk.dart';
import 'package:test/test.dart';

void main() {
  late MathSdk sdk;

  setUpAll(() {
    sdk = MathSdk.open();
  });

  test('version', () {
    expect(sdk.version, isNotEmpty);
  });

  test('evaluate', () {
    expect(sdk.evaluate('2 + 3 * 4'), '14');
    expect(sdk.evaluate('1/3 + 1/6'), '1/2');
  });

  test('evaluate error', () {
    expect(
      () => sdk.evaluate('1/0'),
      throwsA(
        isA<MathSdkException>().having(
          (e) => e.status,
          'status',
          MathSdkStatus.MATHSDK_STATUS_DIVISION_BY_ZERO,
        ),
      ),
    );
  });

  test('evaluateSteps', () {
    final (result, stepsJson) = sdk.evaluateSteps('2 + 3 * 4');
    expect(result, '14');
    expect(stepsJson, contains('"rule"'));
  });

  test('substitute', () {
    expect(
      sdk.substitute(expression: 'x^2 + 2*x + 1', variable: 'x', value: '3'),
      '16',
    );
  });

  test('evaluateNumeric', () {
    expect(sdk.evaluateNumeric('1/4'), closeTo(0.25, 1e-9));
  });

  test('derivative', () {
    expect(sdk.derivative(expression: 'x^3', variable: 'x', order: 2), '6*x');
  });

  test('solve', () {
    final roots = sdk.solve(equation: 'x^2 - 4 = 0', variable: 'x');
    expect(roots, contains('-2'));
    expect(roots, contains('2'));
  });

  test('solveLinearSystem', () {
    final result = sdk.solveLinearSystem(
      equations: '2*x + y = 5;x - y = 1',
      variables: 'x,y',
    );
    expect(result, contains('"x":"2"'));
    expect(result, contains('"y":"1"'));
  });

  test('limit', () {
    expect(sdk.limit(expression: 'sin(x)/x', variable: 'x', target: '0'), '1');
  });

  test('integral indefinite', () {
    expect(sdk.integral(expression: 'x^2', variable: 'x'), '(1/3)*x**3 + C');
  });

  test('integral definite', () {
    expect(
      sdk.integral(
        expression: 'x^2',
        variable: 'x',
        lowerBound: '0',
        upperBound: '3',
      ),
      '9',
    );
  });

  test('series', () {
    expect(
      sdk.series(expression: 'sin(x)', variable: 'x', aroundPoint: '0'),
      contains('O(x**4)'),
    );
  });

  test('matrixOp determinant', () {
    expect(
      sdk.matrixOp(operation: 'determinant', matrixA: '2;2;1,2,3,4'),
      '-2',
    );
  });

  test('matrixOp requires matrixB for multiply', () {
    expect(
      () => sdk.matrixOp(operation: 'multiply', matrixA: '2;2;1,2,3,4'),
      throwsA(isA<MathSdkException>()),
    );
  });

  test('odeSolveSeparable', () {
    expect(
      sdk.odeSolveSeparable(
        fOfX: 'x',
        gOfY: 'y',
        xVariable: 'x',
        yVariable: 'y',
      ),
      'log(y) = (1/2)*x**2 + C',
    );
  });

  test('open returns the same instance within an isolate', () {
    expect(identical(MathSdk.open(), MathSdk.open()), isTrue);
  });

  test('async variants run off the calling isolate', () async {
    expect(await sdk.evaluateAsync('2 + 3 * 4'), '14');
    expect(
      await sdk.derivativeAsync(expression: 'x^3', variable: 'x'),
      '3*x**2',
    );
  });

  test('async variants rethrow MathSdkException', () {
    expect(
      () => sdk.evaluateAsync('1/0'),
      throwsA(
        isA<MathSdkException>().having(
          (e) => e.status,
          'status',
          MathSdkStatus.MATHSDK_STATUS_DIVISION_BY_ZERO,
        ),
      ),
    );
  });

  test('native version matches pubspec', () {
    final declared = File('pubspec.yaml')
        .readAsLinesSync()
        .firstWhere((line) => line.startsWith('version:'))
        .split(':')
        .last
        .trim();
    expect(sdk.version, declared);
  });
}
