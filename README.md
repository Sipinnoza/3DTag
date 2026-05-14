# 3DTag

Native 3D Tag Cloud Engine for Android.

Pure C++ implementation with JNI bridge — sphere rotation, perspective projection, shadow calculation all in native layer.

## Structure

```
src/main/
├── AndroidManifest.xml
├── cpp/
│   ├── CMakeLists.txt        # CMake build config
│   ├── tagcloud3d.h          # Engine header (TagCloudEngine, TagEntry, Point3F)
│   ├── tagcloud3d.cpp        # Engine implementation (rotation, projection, shadow)
│   └── tagcloud3d_jni.cpp    # JNI bridge
└── java/com/znliang/tagcloud3d/
    └── NativeTagCloud.kt     # Kotlin API
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

Android Library module (AAR). Requires:
- Android SDK with NDK 25.1.8937393
- CMake 3.22.1+
- C++17
- Kotlin / Java 11+

Outputs `libtagcloud3d.so` for arm64-v8a and armeabi-v7a.
