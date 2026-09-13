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

class _ResultsList extends StatelessWidget {
  const _ResultsList();

  @override
  Widget build(BuildContext context) {
    final sdk = MathSdk.open();

    final rows = <(String, String)>[
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

    return ListView(
      children: [
        for (final (label, value) in rows)
          ListTile(title: Text(label), subtitle: Text(value)),
      ],
    );
  }
}
