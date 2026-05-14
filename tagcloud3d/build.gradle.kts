plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.znliang.tagcloud3d"
    compileSdk = 35

    defaultConfig {
        minSdk = 24

        consumerProguardFiles("consumer-rules.pro")

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }

        externalNativeBuild {
            cmake {
                cppFlags("-std=c++17")
                arguments(
                    "-DANDROID_STL=c++_static",
                    "-DANDROID_CPPFLAGS=-fno-exceptions+-fno-rtti"
                )
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            externalNativeBuild {
                cmake {
                    // Release 构建用 -Os 最小体积优化
                    cppFlags("-Os", "-DNDEBUG")
                    arguments("-DANDROID_STL=c++_static")
                }
            }
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
