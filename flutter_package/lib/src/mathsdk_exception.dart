import 'bindings_generated.dart';

final class MathSdkException implements Exception {
  const MathSdkException({required this.status, required this.message});

  final MathSdkStatus status;
  final String message;

  @override
  String toString() => 'MathSdkException(${status.name}): $message';
}
