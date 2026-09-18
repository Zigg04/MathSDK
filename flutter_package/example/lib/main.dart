import 'package:flutter/material.dart';
import 'package:mathsdk/mathsdk.dart';

void main() => runApp(const MathSdkExampleApp());

class MathSdkExampleApp extends StatelessWidget {
  const MathSdkExampleApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('mathsdk example')),
        body: const _ResultsList(),
      ),
    );
  }
}

/// One row of the demo: the call as written, and what it returned.
typedef _Row = (String label, String value);

class _ResultsList extends StatefulWidget {
  const _ResultsList();

  @override
  State<_ResultsList> createState() => _ResultsListState();
}

class _ResultsListState extends State<_ResultsList> {
  /// Computed once rather than in `build`, which runs again on every rebuild.
  /// These particular calls are fast enough to run on the UI isolate — see
  /// the cost table on `MathSdkAsync` for when they are not.
  late final List<_Row> _rows;

  /// Set when the native library fails to load or a call fails, so the demo
  /// shows what went wrong instead of dying on a blank screen.
  String? _error;

  @override
  void initState() {
    super.initState();
    try {
      _rows = _compute();
    } on MathSdkException catch (e) {
      _rows = const [];
      _error = e.message;
    } on ArgumentError catch (e) {
      // DynamicLibrary.open throws this when libmathsdk.so is missing.
      _rows = const [];
      _error = 'Could not load the native library: ${e.message}';
    }
  }

  static List<_Row> _compute() {
    final sdk = MathSdk.open();
    return [
      ('version', sdk.version),
      ('evaluate("2 + 3 * 4")', sdk.evaluate('2 + 3 * 4')),
      (
        'derivative(x^3, x, order: 2)',
        sdk.derivative(expression: 'x^3', variable: 'x', order: 2),
      ),
      (
        'solve(x^2 - 4 = 0, x)',
        sdk.solve(equation: 'x^2 - 4 = 0', variable: 'x'),
      ),
      (
        'limit(sin(x)/x, x -> 0)',
        sdk.limit(expression: 'sin(x)/x', variable: 'x', target: '0'),
      ),
      (
        'integral(x^2, x, 0..3)',
        sdk.integral(
          expression: 'x^2',
          variable: 'x',
          lowerBound: '0',
          upperBound: '3',
        ),
      ),
    ];
  }

  @override
  Widget build(BuildContext context) {
    final error = _error;
    if (error != null) {
      return Padding(
        padding: const EdgeInsets.all(24),
        child: Center(child: Text(error, textAlign: TextAlign.center)),
      );
    }
    return ListView.builder(
      itemCount: _rows.length,
      itemBuilder: (context, index) {
        final (label, value) = _rows[index];
        return ListTile(title: Text(label), subtitle: Text(value));
      },
    );
  }
}
