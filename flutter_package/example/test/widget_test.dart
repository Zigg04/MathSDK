import 'package:flutter_test/flutter_test.dart';

import 'package:mathsdk_example/main.dart';

void main() {
  testWidgets('renders without throwing', (WidgetTester tester) async {
    await tester.pumpWidget(const MathSdkExampleApp());
    expect(find.text('mathsdk example'), findsOneWidget);
  });
}
