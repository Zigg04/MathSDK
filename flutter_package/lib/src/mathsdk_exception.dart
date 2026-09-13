import 'bindings_generated.dart';

/// Thrown when a MathSDK native call returns a non-OK [MathSdkStatus].
final class MathSdkException implements Exception {
  const MathSdkException({required this.status, required this.message});

  /// The native status code that triggered this exception.
  final MathSdkStatus status;

  /// A human-readable description of the failure.
  final String message;

  @override
  String toString() => 'MathSdkException(${status.name}): $message';
}
