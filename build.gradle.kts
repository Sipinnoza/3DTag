plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.znliang.tagcloud3d"
    compileSdk = 36

    defaultConfig {
        minSdk = 30

        consumerProguardFiles("consumer-rules.pro")

        // NDK 构建配置：只打 arm64-v8a 和 armeabi-v7a（覆盖绝大多数设备）
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }

        externalNativeBuild {
            cmake {
                cppFlags("-std=c++17")
                arguments("-DANDROID_STL=c++_static")  // 静态链接 STL，so 自包含
            }
        }

        ndkVersion = "25.1.8937393"
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    kotlinOptions {
        jvmTarget = "11"
    }
}

dependencies {
    implementation(libs.androidx.appcompat)
}
