import java.net.URI
import java.security.MessageDigest

plugins {
    id("com.android.library")
}

// MathSDK's native sources live in the repo root, above this directory. They
// are present when developing in the repo or consuming this package as a
// `path:` dependency, and absent when it is installed from pub.dev (the
// published archive contains only Dart sources and this Android project).
//
// Sources present -> build libmathsdk.so from source, so local native changes
// are picked up. Sources absent -> download the prebuilt .so for each ABI
// from the matching GitHub release into jniLibs.
val nativeSourceRoot = file("../..")
val buildFromSource = File(nativeSourceRoot, "CMakeLists.txt").exists()

// pubspec.yaml is the single source of truth for the version: it names the
// release tag the prebuilt libraries are attached to, and it is what the
// native build stamps into math_sdk_version(). Reading it here keeps the two
// from drifting apart.
val mathsdkVersion: String = file("../pubspec.yaml").readLines()
    .firstOrNull { it.startsWith("version:") }
    ?.substringAfter("version:")
    ?.trim()
    ?: throw GradleException("MathSDK: no `version:` found in pubspec.yaml.")
val supportedAbis = listOf("arm64-v8a", "armeabi-v7a", "x86_64")
// Deliberately outside build/: `flutter clean` and `flutter build` wipe that
// directory, which would re-download several MB per ABI on every build. Under
// the Gradle home the libraries are cached across builds and shared by every
// project on this machine that uses the same MathSDK version.
val prebuiltDir: Provider<Directory> = layout.dir(
    providers.provider { File(gradle.gradleUserHomeDir, "caches/mathsdk/$mathsdkVersion") },
)

// Which ABIs this build actually needs. Flutter passes the target ABIs via
// -Ptarget-platform; a plain Gradle build may set abiFilters instead. When
// neither says, fall back to every supported ABI.
val requestedAbis: List<String> by lazy {
    val fromFlutter = (project.findProperty("target-platform") as String?)
        ?.split(",")
        ?.mapNotNull {
            when (it.trim()) {
                "android-arm64" -> "arm64-v8a"
                "android-arm" -> "armeabi-v7a"
                "android-x64" -> "x86_64"
                else -> null
            }
        }
        ?.takeIf { it.isNotEmpty() }
    val abis = fromFlutter ?: supportedAbis
    val unsupported = abis - supportedAbis.toSet()
    if (unsupported.isNotEmpty()) {
        throw GradleException(
            "MathSDK has no prebuilt library for: ${unsupported.joinToString(", ")}.\n" +
                "Supported ABIs are ${supportedAbis.joinToString(", ")}.",
        )
    }
    abis
}

val releaseBaseUrl = "https://github.com/Zigg04/MathSDK/releases/download/v$mathsdkVersion"

fun download(url: String, into: File, what: String) {
    try {
        URI(url).toURL().openStream().use { input ->
            into.outputStream().use { input.copyTo(it) }
        }
    } catch (e: Exception) {
        into.delete()
        throw GradleException(
            "MathSDK: could not download $what.\n" +
                "  URL:   $url\n" +
                "  Error: ${e.message}\n" +
                "Check that release v$mathsdkVersion exists and that this machine has network access.",
            e,
        )
    }
}

fun File.sha256(): String {
    val digest = MessageDigest.getInstance("SHA-256")
    inputStream().use { input ->
        val buffer = ByteArray(1 shl 16)
        while (true) {
            val read = input.read(buffer)
            if (read <= 0) break
            digest.update(buffer, 0, read)
        }
    }
    return digest.digest().joinToString("") { byte -> "%02x".format(byte) }
}

val fetchPrebuiltNativeLibs by tasks.registering {
    description = "Downloads and verifies the prebuilt libmathsdk.so for each requested ABI."
    onlyIf { !buildFromSource }
    // No outputs.dir(prebuiltDir): that directory is a shared cache under the
    // Gradle home, and declaring it as a task output lets Gradle delete it.
    // The task decides for itself what to re-fetch, by checksum.
    doLast {
        // SHA256SUMS is published alongside the libraries by the release
        // workflow. Verifying against it catches a truncated or proxied
        // download, which would otherwise be packaged into the APK and only
        // fail at runtime on the user's device with UnsatisfiedLinkError.
        val sumsFile = prebuiltDir.get().file("SHA256SUMS").asFile
        sumsFile.parentFile.mkdirs()
        if (!sumsFile.exists() || sumsFile.length() == 0L) {
            logger.lifecycle("MathSDK: downloading checksums for v$mathsdkVersion")
            download("$releaseBaseUrl/SHA256SUMS", sumsFile, "the checksum list")
        }
        val expected = sumsFile.readLines()
            .mapNotNull { line ->
                val parts = line.trim().split(Regex("\\s+"), limit = 2)
                if (parts.size == 2) parts[1].removePrefix("*") to parts[0].lowercase() else null
            }
            .toMap()

        requestedAbis.forEach { abi ->
            val name = "libmathsdk-android-$abi.so"
            val target = prebuiltDir.get().file("$abi/libmathsdk.so").asFile
            val wanted = expected[name]
                ?: throw GradleException(
                    "MathSDK: release v$mathsdkVersion publishes no checksum for $name.",
                )
            if (target.exists() && target.sha256() == wanted) return@forEach

            target.parentFile.mkdirs()
            logger.lifecycle("MathSDK: downloading prebuilt for $abi")
            download("$releaseBaseUrl/$name", target, "the prebuilt library for $abi")

            val actual = target.sha256()
            if (actual != wanted) {
                target.delete()
                throw GradleException(
                    "MathSDK: the downloaded library for $abi is corrupt.\n" +
                        "  expected sha256: $wanted\n" +
                        "  actual sha256:   $actual\n" +
                        "The download was truncated or altered in transit. Retry the build.",
                )
            }
        }
    }
}

android {
    namespace = "com.zigg04.mathsdk"
    compileSdk = flutter.compileSdkVersion
    ndkVersion = flutter.ndkVersion

    defaultConfig {
        minSdk = 24
        if (buildFromSource) {
            externalNativeBuild {
                cmake {
                    arguments += "-DANDROID_STL=c++_static"
                }
            }
        }
    }

    if (buildFromSource) {
        externalNativeBuild {
            cmake {
                path = file("CMakeLists.txt")
            }
        }
    } else {
        sourceSets["main"].jniLibs.srcDir(prebuiltDir)
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

if (!buildFromSource) {
    tasks.named("preBuild") { dependsOn(fetchPrebuiltNativeLibs) }
}
