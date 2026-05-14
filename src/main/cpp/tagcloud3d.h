#pragma once

#include <vector>
#include <cmath>
#include <cstddef>

namespace tagcloud3d {

// 3D点，对应 Kotlin 的 Point3DF
struct Point3F {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    Point3F() = default;
    Point3F(float x, float y, float z) : x(x), y(y), z(z) {}
};

// 单个标签数据，对应 Kotlin 的 Tag
struct TagEntry {
    // 三维空间坐标
    float spatialX = 0.f;
    float spatialY = 0.f;
    float spatialZ = 0.f;

    // 二维投影坐标
    float flatX = 0.f;
    float flatY = 0.f;

    // 缩放比例（透视投影产生）
    float scale = 1.f;

    // 透明度 0~1
    float alpha = 1.f;

    // ARGB 颜色分量 0~1
    float colorA = 1.f;
    float colorR = 0.5f;
    float colorG = 0.5f;
    float colorB = 0.5f;

    // ─── 正上方投影（点光源从 Y 轴正方向投射到球体底部平面） ───
    // 投影后的 2D 屏幕偏移
    float shadowFlatX = 0.f;
    float shadowFlatY = 0.f;
    // 投影缩放（越高越大的虚影）
    float shadowScale = 1.f;
    // 投影透明度（越高越淡）
    float shadowAlpha = 0.f;

    // 热度，用于颜色渐变
    int popularity = 0;
};

// 核心计算引擎，对应 Kotlin 的 TagCloud（纯计算，无 Android 依赖）
class TagCloudEngine {
public:
    TagCloudEngine() = default;

    // ─── 配置 ───

    void setRadius(int radius) { radius_ = radius; }

    void setTagColorDark(const float dark[4]) {
        for (int i = 0; i < 4; ++i) darkColor_[i] = dark[i];
    }

    void setTagColorLight(const float light[4]) {
        for (int i = 0; i < 4; ++i) lightColor_[i] = light[i];
    }

    void setInertia(float inertiaX, float inertiaY) {
        inertiaX_ = inertiaX;
        inertiaY_ = inertiaY;
    }

    /**
     * 设置光源角度（度数）。
     * azimuth: 水平旋转角，0 = 正上方偏后，90 = 从右侧照来
     * elevation: 俯仰角，90 = 正上方，0 = 水平
     */
    void setLightAngle(float azimuth, float elevation) {
        lightAzimuth_ = azimuth;
        lightElevation_ = elevation;
    }

    // ─── 标签管理 ───

    void clear() { tags_.clear(); }

    size_t tagCount() const { return tags_.size(); }

    const std::vector<TagEntry>& tags() const { return tags_; }

    TagEntry& tagAt(size_t index) { return tags_[index]; }
    const TagEntry& tagAt(size_t index) const { return tags_[index]; }

    // 添加一个标签（随机球面位置）
    void addTag(int popularity);

    // 使用黄金角分布将所有标签均匀分布到球面
    void createSurfaceDistribution();

    // 根据 popularity 范围重新计算所有标签颜色
    void recalculateColors();

    // ─── 每帧更新 ───

    // 仅当惯性足够大时执行旋转+投影，返回 true 表示实际更新了
    bool update();

    // 获取上一次 update() 是否实际执行了旋转
    bool isDirty() const { return dirty_; }

private:
    std::vector<TagEntry> tags_;
    int radius_ = 3;

    float inertiaX_ = 0.5f;
    float inertiaY_ = 0.5f;

    // 旋转计算的三角函数缓存
    float sinX_ = 0.f, cosX_ = 0.f;
    float sinY_ = 0.f, cosY_ = 0.f;

    // 颜色配置
    float darkColor_[4] = {0.886f, 0.725f, 0.188f, 1.f};
    float lightColor_[4] = {0.3f, 0.3f, 0.3f, 1.f};

    // popularity 范围缓存
    int minPopularity_ = 0;
    int maxPopularity_ = 0;

    bool dirty_ = false;

    // 光源角度（度数），默认正上方
    float lightAzimuth_ = 0.f;      // 水平旋转角
    float lightElevation_ = 90.f;   // 俯仰角，90 = 正上方

    // 复用缓冲区，避免每帧 malloc
    std::vector<float> deltaBuffer_;
    float rotResult_[3] = {0.f, 0.f, 0.f};

    // ─── 内部计算 ───

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
