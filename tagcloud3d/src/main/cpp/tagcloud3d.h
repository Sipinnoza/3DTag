#pragma once

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

namespace tagcloud3d {

// 3D点
struct Point3F {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    Point3F() = default;
    Point3F(float x, float y, float z) : x(x), y(y), z(z) {}
};

// 单个标签数据
struct TagEntry {
    float spatialX = 0.f;
    float spatialY = 0.f;
    float spatialZ = 0.f;
    float flatX = 0.f;
    float flatY = 0.f;
    float scale = 1.f;
    float alpha = 1.f;
    float colorA = 1.f;
    float colorR = 0.5f;
    float colorG = 0.5f;
    float colorB = 0.5f;
    float shadowFlatX = 0.f;
    float shadowFlatY = 0.f;
    float shadowScale = 1.f;
    float shadowAlpha = 0.f;
    int popularity = 0;
};

// 动态数组（替代 std::vector，避免依赖 C++ STL）
template<typename T>
struct Array {
    T* data = nullptr;
    size_t size = 0;
    size_t capacity = 0;

    Array() = default;
    ~Array() { free(data); }

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    T& operator[](size_t i) { return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }

    void push_back(const T& val) {
        if (size >= capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            data = static_cast<T*>(realloc(data, capacity * sizeof(T)));
        }
        data[size++] = val;
    }

    void clear() { size = 0; }

    T* begin() { return data; }
    T* end() { return data + size; }
    const T* begin() const { return data; }
    const T* end() const { return data + size; }

    bool empty() const { return size == 0; }

    void resize(size_t newSize) {
        if (newSize > capacity) {
            capacity = newSize;
            data = static_cast<T*>(realloc(data, capacity * sizeof(T)));
        }
        size = newSize;
    }
};

// 核心计算引擎（纯 C 风格，无 C++ STL 依赖）
class TagCloudEngine {
public:
    TagCloudEngine() = default;
    ~TagCloudEngine();

    TagCloudEngine(const TagCloudEngine&) = delete;
    TagCloudEngine& operator=(const TagCloudEngine&) = delete;

    void setRadius(int radius) { radius_ = radius; }
    void setTagColorDark(const float dark[4]);
    void setTagColorLight(const float light[4]);
    void setInertia(float inertiaX, float inertiaY);
    void setLightAngle(float azimuth, float elevation);

    void clear();
    size_t tagCount() const { return tags_.size; }
    const Array<TagEntry>& tags() const { return tags_; }
    TagEntry& tagAt(size_t index) { return tags_[index]; }

    void addTag(int popularity);
    void createSurfaceDistribution();
    void recalculateColors();

    bool update();
    bool isDirty() const { return dirty_; }

private:
    Array<TagEntry> tags_;
    int radius_ = 3;
    float inertiaX_ = 0.5f;
    float inertiaY_ = 0.5f;
    float sinX_ = 0.f, cosX_ = 0.f;
    float sinY_ = 0.f, cosY_ = 0.f;
    float darkColor_[4] = {0.886f, 0.725f, 0.188f, 1.f};
    float lightColor_[4] = {0.3f, 0.3f, 0.3f, 1.f};
    int minPopularity_ = 0;
    int maxPopularity_ = 0;
    bool dirty_ = false;
    float lightAzimuth_ = 0.f;
    float lightElevation_ = 90.f;

    // 复用缓冲区
    float* deltaBuffer_ = nullptr;
    size_t deltaBufCap_ = 0;
    float rotResult_[3] = {0.f, 0.f, 0.f};

    void initTag(TagEntry& tag);
    float getPercentage(int popularity) const;
    void positionRandom(TagEntry& tag);
    void updateAll();
    void applyRotation(float x, float y, float z);
    void recalculateAngle();
    void getColorFromGradient(float percentage, float out[4]) const;
    void updateTagProjection(TagEntry& tag);
    void computeTopDownShadow();
};

} // namespace tagcloud3d
