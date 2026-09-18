import 'package:flutter_test/flutter_test.dart';
import 'package:mathsdk_example/main.dart';

void main() {
  testWidgets('shows results, or says why it cannot', (tester) async {
    await tester.pumpWidget(const MathSdkExampleApp());
    await tester.pumpAndSettle();

    expect(find.text('mathsdk example'), findsOneWidget);

    // `flutter test` runs on the host, where the native library is only
    // present if one was built for this machine. Both outcomes are correct;
    // what must never happen is a blank screen or an uncaught exception.
    if (find.textContaining('Could not load').evaluate().isNotEmpty) {
      return;
    }

    expect(find.text('evaluate("2 + 3 * 4")'), findsOneWidget);
    expect(find.text('14'), findsOneWidget);
  });
}
