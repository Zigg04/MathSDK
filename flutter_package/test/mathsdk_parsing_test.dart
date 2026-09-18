import 'package:mathsdk/mathsdk.dart';
import 'package:test/test.dart';

void main() {
  late MathSdk sdk;

  setUpAll(() {
    sdk = MathSdk.open();
  });

  group('typed accessors', () {
    test('solveRoots returns the roots as a list', () {
      final roots = sdk.solveRoots(equation: 'x^2 - 4 = 0', variable: 'x');
      expect(roots, unorderedEquals(['2', '-2']));
    });

    test('solveSystem returns a variable map', () {
      expect(
        sdk.solveSystem(equations: '2*x + y = 5;x - y = 1', variables: 'x,y'),
        {'x': '2', 'y': '1'},
      );
    });

    test('matrix parses dimensions and entries', () {
      final inverse = sdk.matrix(operation: 'inverse', matrixA: '2;2;1,2,3,4');
      expect(inverse.rowCount, 2);
      expect(inverse.columnCount, 2);
      expect(inverse[(row: 0, column: 0)], '-2');
      expect(inverse[(row: 1, column: 1)], '-1/2');
      expect(inverse.rows, [
        ['-2', '1'],
        ['3/2', '-1/2'],
      ]);
    });

    test('matrix round-trips through encode', () {
      final matrix = MathMatrix.parse('2;2;1,2,3,4');
      expect(matrix.encode(), '2;2;1,2,3,4');
      expect(
        sdk.matrix(operation: 'transpose', matrixA: matrix.encode()).encode(),
        '2;2;1,3,2,4',
      );
    });

    test('evaluateWithSteps parses the reduction trace', () {
      final (result, steps) = sdk.evaluateWithSteps('2 + 3 * 4');
      expect(result, '14');
      expect(steps, isNotEmpty);
      expect(steps.first.rule, isNotEmpty);
      expect(steps.last.after, '14');
    });
  });

  group('MathMatrix.parse', () {
    test('accepts a single entry', () {
      final matrix = MathMatrix.parse('1;1;7');
      expect(matrix.values, ['7']);
      expect(matrix[(row: 0, column: 0)], '7');
    });

    test('keeps symbolic entries verbatim', () {
      expect(MathMatrix.parse('1;2;x + 1,2/3').values, ['x + 1', '2/3']);
    });

    test('rejects a missing section', () {
      expect(
        () => MathMatrix.parse('2;2'),
        throwsA(
          isA<MathSdkException>().having(
            (e) => e.status,
            'status',
            MathSdkStatus.MATHSDK_STATUS_PARSE_ERROR,
          ),
        ),
      );
    });

    test('rejects non-integer dimensions', () {
      expect(
        () => MathMatrix.parse('a;2;1,2'),
        throwsA(isA<MathSdkException>()),
      );
    });

    test('rejects a wrong entry count', () {
      expect(
        () => MathMatrix.parse('2;2;1,2,3'),
        throwsA(
          isA<MathSdkException>().having(
            (e) => e.message,
            'message',
            contains('Expected 4 entries'),
          ),
        ),
      );
    });

    test('exposed collections are unmodifiable', () {
      final matrix = MathMatrix.parse('1;2;1,2');
      expect(() => matrix.values.add('3'), throwsUnsupportedError);
      expect(() => matrix.rows.first.add('3'), throwsUnsupportedError);
    });

    test('copies the caller list instead of aliasing it', () {
      final source = ['1', '2'];
      final matrix = MathMatrix(rowCount: 1, columnCount: 2, values: source);
      source[0] = 'mutated';
      expect(matrix.values, ['1', '2']);
    });

    test('rejects out-of-range access', () {
      final matrix = MathMatrix.parse('1;1;7');
      expect(() => matrix[(row: 1, column: 0)], throwsRangeError);
      expect(() => matrix[(row: 0, column: 1)], throwsRangeError);
    });
  });
}
