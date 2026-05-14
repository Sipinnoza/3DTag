#include "tagcloud3d.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tagcloud3d {

// ─── 标签管理 ───

void TagCloudEngine::addTag(int popularity) {
    TagEntry tag;
    tag.popularity = popularity;
    initTag(tag);
    positionRandom(tag);
    tags_.push_back(tag);
    updateAll();
}

void TagCloudEngine::createSurfaceDistribution() {
    const size_t count = tags_.size();
    if (count == 0) return;

    // 按 popularity 降序排序，常用标签放正面
    std::vector<size_t> indices(count);
    for (size_t i = 0; i < count; ++i) indices[i] = i;
    std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
        return tags_[a].popularity > tags_[b].popularity;
    });

    const double goldenRatio = (1.0 + std::sqrt(5.0)) / 2.0;
    const double PI = 3.14159265358979323846;
    const int denom = static_cast<int>(count - 1);
    const int safeDenom = denom > 0 ? denom : 1;

    for (size_t i = 0; i < count; ++i) {
        auto& tag = tags_[indices[i]];
        const double theta = std::acos(1.0 - 2.0 * static_cast<double>(i) / safeDenom);
        const double phi = static_cast<double>(i) * goldenRatio * 2.0 * PI;
        tag.spatialX = static_cast<float>(radius_ * std::sin(theta) * std::cos(phi));
        tag.spatialY = static_cast<float>(radius_ * std::sin(theta) * std::sin(phi));
        tag.spatialZ = static_cast<float>(radius_ * std::cos(theta));
        updateTagProjection(tag);
    }
}

void TagCloudEngine::recalculateColors() {
    if (tags_.empty()) return;
    minPopularity_ = tags_[0].popularity;
    maxPopularity_ = tags_[0].popularity;
    for (const auto& t : tags_) {
        if (t.popularity < minPopularity_) minPopularity_ = t.popularity;
        if (t.popularity > maxPopularity_) maxPopularity_ = t.popularity;
    }
    for (auto& t : tags_) {
        initTag(t);
    }
}

// ─── 每帧更新 ───

bool TagCloudEngine::update() {
    if (std::fabs(inertiaX_) > 0.1f || std::fabs(inertiaY_) > 0.1f) {
        recalculateAngle();
        updateAll();
        dirty_ = true;
    } else {
        dirty_ = false;
    }
    return dirty_;
}

// ─── 内部计算 ───

void TagCloudEngine::initTag(TagEntry& tag) {
    const float pct = getPercentage(tag.popularity);
    float argb[4];
    getColorFromGradient(pct, argb);
    tag.colorA = argb[0];
    tag.colorR = argb[1];
    tag.colorG = argb[2];
    tag.colorB = argb[3];
}

float TagCloudEngine::getPercentage(int popularity) const {
    if (minPopularity_ == maxPopularity_) return 1.f;
    return static_cast<float>(popularity - minPopularity_)
         / static_cast<float>(maxPopularity_ - minPopularity_);
}

void TagCloudEngine::positionRandom(TagEntry& tag) {
    constexpr double PI = 3.14159265358979323846;
    const double phi = std::rand() / static_cast<double>(RAND_MAX) * PI;
    const double theta = std::rand() / static_cast<double>(RAND_MAX) * 2.0 * PI;
    tag.spatialX = static_cast<float>(radius_ * std::cos(theta) * std::sin(phi));
    tag.spatialY = static_cast<float>(radius_ * std::sin(theta) * std::sin(phi));
    tag.spatialZ = static_cast<float>(radius_ * std::cos(phi));
}

void TagCloudEngine::updateAll() {
    const float diameter = static_cast<float>(2 * radius_);
    const float projectionDepth = diameter * 1.3f;
    const size_t count = tags_.size();

    // 复用 delta 缓冲区
    if (deltaBuffer_.size() < count) {
        deltaBuffer_.resize(count);
    }

    float maxDelta = std::numeric_limits<float>::min();
    float minDelta = std::numeric_limits<float>::max();

    // 单遍：旋转 + 投影 + 收集 delta
    for (size_t i = 0; i < count; ++i) {
        auto& tag = tags_[i];
        applyRotation(tag.spatialX, tag.spatialY, tag.spatialZ);
        tag.spatialX = rotResult_[0];
        tag.spatialY = rotResult_[1];
        tag.spatialZ = rotResult_[2];

        const float per = projectionDepth / (projectionDepth + tag.spatialZ);
        tag.flatX = tag.spatialX * per;
        tag.flatY = tag.spatialY * per;
        tag.scale = per;

        const float delta = projectionDepth + tag.spatialZ;
        deltaBuffer_[i] = delta;
        if (delta > maxDelta) maxDelta = delta;
        if (delta < minDelta) minDelta = delta;
    }

    // 第二遍：alpha
    const float range = maxDelta - minDelta;
    if (range > 0.f) {
        for (size_t i = 0; i < count; ++i) {
            const float raw = 1.f - (deltaBuffer_[i] - minDelta) / range;
            tags_[i].alpha = 0.1f + raw * 0.6f;
        }
    }

    // ─── 第三遍：正上方投影（点光源投射到球体底部平面） ───
    // 光源在 (0, lightHeight, 0)，投影平面 y = -radius_
    // 影子会出现在标签云下方，标签越高影子越大越淡
    computeTopDownShadow();
}

/**
 * 透视投影影子：从标签3D位置沿光线投射到地面平面 (y = -radius)，
 * 然后映射到2D屏幕坐标。光源角度可自定义。
 *
 * 光源位置由 lightAzimuth_（水平旋转角）和 lightElevation_（俯仰角）决定：
 *   lightX = lightDist * cos(elevation) * sin(azimuth)
 *   lightY = lightDist * sin(elevation)
 *   lightZ = lightDist * cos(elevation) * cos(azimuth)
 * 投影到地面平面 y = -r：
 *   t = (lightY + r) / (lightY - tag.spatialY)
 *   projX = lightX + t * (tag.spatialX - lightX)
 *   projZ = lightZ + t * (tag.spatialZ - lightZ)
 */
void TagCloudEngine::computeTopDownShadow() {
    const size_t count = tags_.size();
    if (count == 0 || radius_ <= 0) return;

    const float r = static_cast<float>(radius_);
    constexpr float degToRad = 3.14159265f / 180.f;

    // 光源距离球心：球顶上方 1.5 倍半径
    const float lightDist = r * 2.5f;
    const float elevRad = lightElevation_ * degToRad;
    const float azimRad = lightAzimuth_ * degToRad;
    const float cosElev = cosf(elevRad);
    const float sinElev = sinf(elevRad);

    const float lightX = lightDist * cosElev * sinf(azimRad);
    const float lightY = lightDist * sinElev;
    const float lightZ = lightDist * cosElev * cosf(azimRad);

    // 地面到球心的屏幕距离
    const float groundBase = r * 0.75f;

    for (size_t i = 0; i < count; ++i) {
        const auto& tag = tags_[i];

        // 标签在球背面不画影子
        if (tag.alpha < 0.05f) {
            tags_[i].shadowAlpha = 0.f;
            continue;
        }

        // ── 透视投影到地面 y = -r ──
        const float denom = lightY - tag.spatialY;
        float t = 1.f;
        if (fabsf(denom) > 0.001f) {
            t = (lightY + r) / denom;
        }
        const float projX = lightX + t * (tag.spatialX - lightX);
        const float projZ = lightZ + t * (tag.spatialZ - lightZ);

        // 映射到屏幕偏移（以球心为原点）
        tags_[i].shadowFlatX = projX * (r / (r + fabsf(projX) * 0.3f));
        tags_[i].shadowFlatY = groundBase + projZ * 0.15f;

        // 高度归一化：0（球底）到 1（球顶）
        const float normalizedHeight = (r - tag.spatialY) / (2.f * r);

        // 越高影子越大，越低影子越小
        tags_[i].shadowScale = 0.5f + normalizedHeight * 0.6f;
        // 越高影子越淡，越低越浓
        tags_[i].shadowAlpha = 0.25f * (1.f - normalizedHeight * 0.7f);
    }
}

void TagCloudEngine::applyRotation(float x, float y, float z) {
    // 绕 X 轴旋转
    const float ry1 = y * cosX_ - z * sinX_;
    const float rz1 = y * sinX_ + z * cosX_;
    // 绕 Y 轴旋转
    rotResult_[0] = x * cosY_ + rz1 * sinY_;
    rotResult_[1] = ry1;
    rotResult_[2] = -x * sinY_ + rz1 * cosY_;
}

void TagCloudEngine::recalculateAngle() {
    constexpr float degToRad = 3.14159265f / 180.f;
    sinX_ = std::sin(inertiaX_ * degToRad);
    cosX_ = std::cos(inertiaX_ * degToRad);
    sinY_ = std::sin(inertiaY_ * degToRad);
    cosY_ = std::cos(inertiaY_ * degToRad);
}

void TagCloudEngine::getColorFromGradient(float percentage, float out[4]) const {
    out[0] = 1.f;
    for (int i = 1; i < 4; ++i) {
        out[i] = percentage * darkColor_[i] + (1.f - percentage) * lightColor_[i];
    }
}

void TagCloudEngine::updateTagProjection(TagEntry& tag) {
    const float depthFactor = (radius_ + tag.spatialZ) / (2.f * radius_);
    tag.flatX = tag.spatialX * depthFactor;
    tag.flatY = tag.spatialY * depthFactor;
    tag.scale = std::max(0.5f, std::min(1.5f, depthFactor));
}

} // namespace tagcloud3d
