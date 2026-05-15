cmake_minimum_required(VERSION 3.22.1)

project("tagcloud3d")

# 用 C++ 编译器但写 C 风格代码，不依赖任何 STL
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 不链 C++ STL，不链异常/RTTI，去掉 unwind 表
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -fno-unwind-tables -ffunction-sections -fdata-sections")

# Release: 最小体积优化 + LTO
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -flto=thin")
endif()

add_library(
    tagcloud3d
    SHARED
    tagcloud3d.cpp
    tagcloud3d_jni.cpp
)

# 链接时丢弃未使用 section
target_link_options(tagcloud3d PRIVATE -Wl,--gc-sections)

target_link_libraries(
    tagcloud3d
    log
)
