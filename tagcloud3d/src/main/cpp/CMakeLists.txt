cmake_minimum_required(VERSION 3.22.1)

project("tagcloud3d")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 减小 so 体积：关闭异常、RTTI、禁用 unwind tables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -fno-unwind-tables -ffunction-sections -fdata-sections")

# Release 构建：最大体积优化 + LTO
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -flto=thin")
endif()

add_library(
    tagcloud3d
    SHARED
    tagcloud3d.cpp
    tagcloud3d_jni.cpp
)

# 链接时丢弃未使用的 section（-ffunction-sections 配合 --gc-sections）
target_link_options(tagcloud3d PRIVATE -Wl,--gc-sections)

# Release 构建时去掉 log 依赖中不需要的符号
target_link_libraries(
    tagcloud3d
    log
)
