cmake_minimum_required(VERSION 3.22.1)

project("tagcloud3d")

# C++17 用于结构化绑定等现代特性
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 关闭异常和 RTTI（减小 so 体积）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

add_library(
    tagcloud3d
    SHARED
    tagcloud3d.cpp
    tagcloud3d_jni.cpp
)

# 链接 Android log 库（用于 __android_log_print）
target_link_libraries(
    tagcloud3d
    log
)
