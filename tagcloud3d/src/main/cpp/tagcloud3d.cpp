#include "tagcloud3d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace tagcloud3d {

TagCloudEngine::~TagCloudEngine() {
    free(deltaBuffer_);
}

// ─── 标签管理 ───

void TagCloudEngine::addTag(int popularity) {
    TagEntry tag;
    tag.popularity = popularity;
    initTag(tag);
    positionRandom(tag);
    tags_.push_back(tag);
    updateAll();
}

// qsort 比较函数（替代 std::sort lambda）
struct SortCtx {
    const TagEntry* tags;
    static int compare(const void* a, const void* b) {
        int pa = static_cast<const SortCtx*>(a)->tags[*static_cast<const size_t*>(a)].popularity;  // unused
        // We don't use this approach; use a simpler method below
        return 0;
    }
};

// 简单冒泡排序（标签数量少，不需要复杂排序）
static void sortIndicesByPopularity(size_t* indices, size_t count, const TagEntry* tags) {
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            if (tags[indices[j]].popularity > tags[indices[i]].popularity) {
                size_t tmp = indices[i];
                indices[i] = indices[j];
                indices[j] = tmp;
            }
        }
    }
}

void TagCloudEngine::createSurfaceDistribution() {
    const size_t count = tags_.size;
    if (count == 0) return;

    // 按热度降序排索引
    size_t* indices = static_cast<size_t*>(malloc(count * sizeof(size_t)));
    for (size_t i = 0; i < count; ++i) indices[i] = i;
    sortIndicesByPopularity(indices, count, tags_.data);

    const double goldenRatio = (1.0 + sqrt(5.0)) / 2.0;
    constexpr double PI = 3.14159265358979323846;
    const int safeDenom = count > 1 ? static_cast<int>(count - 1) : 1;

    for (size_t i = 0; i < count; ++i) {
        auto& tag = tags_[indices[i]];
        const double theta = acos(1.0 - 2.0 * static_cast<double>(i) / safeDenom);
        const double phi = static_cast<double>(i) * goldenRatio * 2.0 * PI;
        tag.spatialX = static_cast<float>(radius_ * sin(theta) * cos(phi));
        tag.spatialY = static_cast<float>(radius_ * sin(theta) * sin(phi));
        tag.spatialZ = static_cast<float>(radius_ * cos(theta));
        updateTagProjection(tag);
    }

    free(indices);
}

void TagCloudEngine::recalculateColors() {
    if (tags_.empty()) return;
    minPopularity_ = tags_[0].popularity;
    maxPopularity_ = tags_[0].popularity;
    for (size_t i = 1; i < tags_.size; ++i) {
        if (tags_[i].popularity < minPopularity_) minPopularity_ = tags_[i].popularity;
        if (tags_[i].popularity > maxPopularity_) maxPopularity_ = tags_[i].popularity;
    }
    for (size_t i = 0; i < tags_.size; ++i) {
        initTag(tags_[i]);
    }
}

// ─── 每帧更新 ───

bool TagCloudEngine::update() {
    if (fabsf(inertiaX_) > 0.1f || fabsf(inertiaY_) > 0.1f) {
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
    const double phi = rand() / static_cast<double>(RAND_MAX) * PI;
    const double theta = rand() / static_cast<double>(RAND_MAX) * 2.0 * PI;
    tag.spatialX = static_cast<float>(radius_ * cos(theta) * sin(phi));
    tag.spatialY = static_cast<float>(radius_ * sin(theta) * sin(phi));
    tag.spatialZ = static_cast<float>(radius_ * cos(phi));
}

void TagCloudEngine::updateAll() {
    const float diameter = static_cast<float>(2 * radius_);
    const float projectionDepth = diameter * 1.3f;
    const size_t count = tags_.size;

    // 扩展 delta 缓冲区
    if (count > deltaBufCap_) {
        deltaBufCap_ = count;
        deltaBuffer_ = static_cast<float*>(realloc(deltaBuffer_, deltaBufCap_ * sizeof(float)));
    }

    float maxDelta = -1e30f;
    float minDelta = 1e30f;

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

    const float range = maxDelta - minDelta;
    if (range > 0.f) {
        for (size_t i = 0; i < count; ++i) {
            const float raw = 1.f - (deltaBuffer_[i] - minDelta) / range;
            tags_[i].alpha = 0.1f + raw * 0.6f;
        }
    }

    computeTopDownShadow();
}

void TagCloudEngine::computeTopDownShadow() {
    const size_t count = tags_.size;
    if (count == 0 || radius_ <= 0) return;

    const float r = static_cast<float>(radius_);
    constexpr float degToRad = 3.14159265f / 180.f;

    const float lightDist = r * 2.5f;
    const float elevRad = lightElevation_ * degToRad;
    const float azimRad = lightAzimuth_ * degToRad;
    const float cosElev = cosf(elevRad);
    const float sinElev = sinf(elevRad);

    const float lightX = lightDist * cosElev * sinf(azimRad);
    const float lightY = lightDist * sinElev;
    const float lightZ = lightDist * cosElev * cosf(azimRad);

    const float groundBase = r * 0.75f;

    for (size_t i = 0; i < count; ++i) {
        const auto& tag = tags_[i];

        if (tag.alpha < 0.05f) {
            tags_[i].shadowAlpha = 0.f;
            continue;
        }

        const float denom = lightY - tag.spatialY;
        float t = 1.f;
        if (fabsf(denom) > 0.001f) {
            t = (lightY + r) / denom;
        }
        const float projX = lightX + t * (tag.spatialX - lightX);
        const float projZ = lightZ + t * (tag.spatialZ - lightZ);

        tags_[i].shadowFlatX = projX * (r / (r + fabsf(projX) * 0.3f));
        tags_[i].shadowFlatY = groundBase + projZ * 0.15f;

        const float normalizedHeight = (r - tag.spatialY) / (2.f * r);

        tags_[i].shadowScale = 0.5f + normalizedHeight * 0.6f;
        tags_[i].shadowAlpha = 0.25f * (1.f - normalizedHeight * 0.7f);
    }
}

void TagCloudEngine::applyRotation(float x, float y, float z) {
    const float ry1 = y * cosX_ - z * sinX_;
    const float rz1 = y * sinX_ + z * cosX_;
    rotResult_[0] = x * cosY_ + rz1 * sinY_;
    rotResult_[1] = ry1;
    rotResult_[2] = -x * sinY_ + rz1 * cosY_;
}

void TagCloudEngine::recalculateAngle() {
    constexpr float degToRad = 3.14159265f / 180.f;
    sinX_ = sinf(inertiaX_ * degToRad);
    cosX_ = cosf(inertiaX_ * degToRad);
    sinY_ = sinf(inertiaY_ * degToRad);
    cosY_ = cosf(inertiaY_ * degToRad);
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
    tag.scale = fmaxf(0.5f, fminf(1.5f, depthFactor));
}

void TagCloudEngine::setTagColorDark(const float dark[4]) {
    memcpy(darkColor_, dark, sizeof(darkColor_));
}

void TagCloudEngine::setTagColorLight(const float light[4]) {
    memcpy(lightColor_, light, sizeof(lightColor_));
}

void TagCloudEngine::setInertia(float inertiaX, float inertiaY) {
    inertiaX_ = inertiaX;
    inertiaY_ = inertiaY;
}

void TagCloudEngine::setLightAngle(float azimuth, float elevation) {
    lightAzimuth_ = azimuth;
    lightElevation_ = elevation;
}

void TagCloudEngine::clear() {
    tags_.clear();
}

} // namespace tagcloud3d
