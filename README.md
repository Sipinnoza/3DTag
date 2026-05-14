# 3DTag

Native 3D Tag Cloud Engine for Android.

Pure C++ implementation with JNI bridge — sphere rotation, perspective projection, shadow calculation all in native layer.

## Structure

```
tagcloud3d/
├── build.gradle.kts          # Android Library module
└── src/main/
    ├── AndroidManifest.xml
    ├── cpp/
    │   ├── CMakeLists.txt    # CMake build config
    │   ├── tagcloud3d.h      # Engine header
    │   ├── tagcloud3d.cpp    # Engine implementation
    │   └── tagcloud3d_jni.cpp # JNI bridge
    └── java/com/znliang/tagcloud3d/
        └── NativeTagCloud.kt # Kotlin API
```

## Usage

```kotlin
val engine = NativeTagCloud()
engine.radius = 300
engine.addTag(popularity = 5)
engine.addTag(popularity = 10)
engine.createSurfaceDistribution()
engine.recalculateColors()

// per frame
engine.inertiaX = 0.3f
engine.inertiaY = 0.5f
val dirty = engine.update()
if (dirty) {
    val data = engine.getTagData() // FloatArray[tagCount * 12]
}
```

## Build

Requirements:
- JDK 17
- Android SDK with NDK (auto-installed by Gradle)
- CMake 3.22.1+
- C++17

```bash
./gradlew :tagcloud3d:assembleRelease
```

Output AAR: `tagcloud3d/build/outputs/aar/tagcloud3d-release.aar`

## CI

GitHub Actions automatically builds on every push to `main`.
Download the AAR and SO files from the **Actions → Artifacts** section.
