#include "tagcloud3d.h"

#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "TagCloud3D"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace tagcloud3d;

static inline jlong engineToJlong(TagCloudEngine* engine) {
    return static_cast<jlong>(reinterpret_cast<intptr_t>(engine));
}

static inline TagCloudEngine* jlongToEngine(jlong handle) {
    return reinterpret_cast<TagCloudEngine*>(static_cast<intptr_t>(handle));
}

extern "C" {

// --- Lifecycle ---

JNIEXPORT jlong JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeCreate(JNIEnv* /*env*/, jobject /*thiz*/) {
    TagCloudEngine* engine = new TagCloudEngine();
    return engineToJlong(engine);
}

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeDestroy(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    TagCloudEngine* engine = jlongToEngine(handle);
    if (engine) delete engine;
}

// --- Config ---

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeSetRadius(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle, jint radius) {
    auto* engine = jlongToEngine(handle);
    if (engine) engine->setRadius(radius);
}

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeSetInertia(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle, jfloat inertiaX, jfloat inertiaY) {
    auto* engine = jlongToEngine(handle);
    if (engine) engine->setInertia(inertiaX, inertiaY);
}

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeSetTagColorDark(JNIEnv* env, jobject /*thiz*/, jlong handle, jfloatArray colors) {
    auto* engine = jlongToEngine(handle);
    if (!engine) return;
    jfloat buf[4];
    env->GetFloatArrayRegion(colors, 0, 4, buf);
    engine->setTagColorDark(buf);
}

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeSetTagColorLight(JNIEnv* env, jobject /*thiz*/, jlong handle, jfloatArray colors) {
    auto* engine = jlongToEngine(handle);
    if (!engine) return;
    jfloat buf[4];
    env->GetFloatArrayRegion(colors, 0, 4, buf);
    engine->setTagColorLight(buf);
}

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeSetLightAngle(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle, jfloat azimuth, jfloat elevation) {
    auto* engine = jlongToEngine(handle);
    if (engine) engine->setLightAngle(azimuth, elevation);
}

// --- Tag management ---

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeClear(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    auto* engine = jlongToEngine(handle);
    if (engine) engine->clear();
}

JNIEXPORT jint JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeAddTag(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle, jint popularity) {
    auto* engine = jlongToEngine(handle);
    if (!engine) return -1;
    engine->addTag(popularity);
    return static_cast<jint>(engine->tagCount() - 1);
}

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeCreateSurfaceDistribution(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    auto* engine = jlongToEngine(handle);
    if (engine) engine->createSurfaceDistribution();
}

JNIEXPORT void JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeRecalculateColors(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    auto* engine = jlongToEngine(handle);
    if (engine) engine->recalculateColors();
}

// --- Per-frame update ---

JNIEXPORT jboolean JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeUpdate(JNIEnv* /*env*/, jobject /*thiz*/, jlong handle) {
    auto* engine = jlongToEngine(handle);
    if (!engine) return JNI_FALSE;
    return engine->update() ? JNI_TRUE : JNI_FALSE;
}

// --- Read tag data (batch) ---

JNIEXPORT jfloatArray JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeGetTagData(JNIEnv* env, jobject /*thiz*/, jlong handle) {
    auto* engine = jlongToEngine(handle);
    if (!engine) return nullptr;

    const auto& tags = engine->tags();
    const size_t count = tags.size;
    constexpr size_t FIELDS = 12;
    const size_t total = count * FIELDS;

    jfloatArray result = env->NewFloatArray(static_cast<jsize>(total));
    if (!result || count == 0) return result;

    float* buf = static_cast<float*>(malloc(total * sizeof(float)));
    for (size_t i = 0; i < count; ++i) {
        const auto& t = tags[i];
        const size_t base = i * FIELDS;
        buf[base + 0] = t.flatX;
        buf[base + 1] = t.flatY;
        buf[base + 2] = t.scale;
        buf[base + 3] = t.alpha;
        buf[base + 4] = t.colorA;
        buf[base + 5] = t.colorR;
        buf[base + 6] = t.colorG;
        buf[base + 7] = t.colorB;
        buf[base + 8] = t.shadowFlatX;
        buf[base + 9] = t.shadowFlatY;
        buf[base + 10] = t.shadowScale;
        buf[base + 11] = t.shadowAlpha;
    }

    env->SetFloatArrayRegion(result, 0, static_cast<jsize>(total), buf);
    free(buf);
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_znliang_tagcloud3d_NativeTagCloud_nativeGetPopularities(JNIEnv* env, jobject /*thiz*/, jlong handle) {
    auto* engine = jlongToEngine(handle);
    if (!engine) return nullptr;

    const auto& tags = engine->tags();
    const size_t count = tags.size;

    jintArray result = env->NewIntArray(static_cast<jsize>(count));
    if (!result || count == 0) return result;

    jint* buf = static_cast<jint*>(malloc(count * sizeof(jint)));
    for (size_t i = 0; i < count; ++i) {
        buf[i] = tags[i].popularity;
    }

    env->SetIntArrayRegion(result, 0, static_cast<jsize>(count), buf);
    free(buf);
    return result;
}

} // extern "C"
