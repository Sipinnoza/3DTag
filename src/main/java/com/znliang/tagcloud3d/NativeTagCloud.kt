package com.znliang.tagcloud3d

import android.graphics.Color

/**
 * Native 3D 标签云计算引擎。
 * 纯 C++ 实现，通过 JNI 调用，无 Android View 依赖。
 *
 * 用法：
 *   val engine = NativeTagCloud()
 *   engine.radius = 300
 *   engine.addTag(popularity = 5)
 *   engine.addTag(popularity = 10)
 *   engine.createSurfaceDistribution()
 *   engine.recalculateColors()
 *
 *   // 每帧
 *   engine.inertiaX = 0.3f
 *   engine.inertiaY = 0.5f
 *   val dirty = engine.update()
 *   if (dirty) {
 *       val data = engine.getTagData()  // FloatArray[tagCount * 12]
 *       // 解析 flatX, flatY, scale, alpha, colorA, colorR, colorG, colorB,
 *       //       shadowFlatX, shadowFlatY, shadowScale, shadowAlpha
 *   }
 */
class NativeTagCloud {

    // native 引擎指针
    private var handle: Long = 0L

    init {
        handle = nativeCreate()
    }

    // ─── 配置属性 ───

    var radius: Int = 3
        set(value) {
            field = value
            nativeSetRadius(handle, value)
        }

    var inertiaX: Float = 0.5f
        set(value) {
            field = value
            nativeSetInertia(handle, field, inertiaY)
        }

    var inertiaY: Float = 0.5f
        set(value) {
            field = value
            nativeSetInertia(handle, inertiaX, field)
        }

    /** 光源水平旋转角（度数），0 = 正后上方，90 = 右侧，默认 0 */
    var lightAzimuth: Float = 0f
        set(value) {
            field = value
            nativeSetLightAngle(handle, field, lightElevation)
        }

    /** 光源俯仰角（度数），90 = 正上方，0 = 水平，默认 90 */
    var lightElevation: Float = 90f
        set(value) {
            field = value
            nativeSetLightAngle(handle, lightAzimuth, field)
        }

    fun setTagColorDark(colors: FloatArray) {
        nativeSetTagColorDark(handle, colors)
    }

    fun setTagColorLight(colors: FloatArray) {
        nativeSetTagColorLight(handle, colors)
    }

    // ─── 标签管理 ───

    val tagCount: Int
        get() = cachedTagData?.let { it.size / FIELDS_PER_TAG } ?: 0

    fun clear() {
        cachedTagData = null
        nativeClear(handle)
    }

    /**
     * 添加一个标签，返回其 index
     */
    fun addTag(popularity: Int): Int {
        return nativeAddTag(handle, popularity)
    }

    fun createSurfaceDistribution() {
        nativeCreateSurfaceDistribution(handle)
    }

    fun recalculateColors() {
        nativeRecalculateColors(handle)
    }

    // ─── 每帧更新 ───

    /**
     * 根据当前 inertia 执行旋转+投影计算。
     * @return true 表示位置发生了变化（dirty）
     */
    fun update(): Boolean {
        val dirty = nativeUpdate(handle)
        if (dirty) cachedTagData = null  // 标记缓存失效
        return dirty
    }

    // ─── 读取数据 ───

    /**
     * 获取所有标签的投影数据。
     * 返回 FloatArray[tagCount * 12]，每个标签 12 个 float：
     *   [flatX, flatY, scale, alpha, colorA, colorR, colorG, colorB,
     *    shadowFlatX, shadowFlatY, shadowScale, shadowAlpha]
     *
     * 结果会被缓存，调用 update() 后失效。
     */
    fun getTagData(): FloatArray {
        cachedTagData?.let { return it }
        val data = nativeGetTagData(handle) ?: return FloatArray(0)
        cachedTagData = data
        return data
    }

    /**
     * 便捷方法：获取第 index 个标签的数据。
     * @return TagData 或 null（越界时）
     */
    fun getTag(index: Int): TagData? {
        val data = getTagData()
        if (data.isEmpty()) return null
        val base = index * FIELDS_PER_TAG
        if (base + FIELDS_PER_TAG > data.size) return null
        return TagData(
            flatX = data[base],
            flatY = data[base + 1],
            scale = data[base + 2],
            alpha = data[base + 3],
            color = Color.argb(
                (data[base + 4] * 255).toInt(),
                (data[base + 5] * 255).toInt(),
                (data[base + 6] * 255).toInt(),
                (data[base + 7] * 255).toInt()
            ),
            shadowFlatX = data[base + 8],
            shadowFlatY = data[base + 9],
            shadowScale = data[base + 10],
            shadowAlpha = data[base + 11]
        )
    }

    // ─── 生命周期 ───

    fun release() {
        if (handle != 0L) {
            nativeDestroy(handle)
            handle = 0L
        }
    }

    protected fun finalize() {
        release()
    }

    // ─── 数据类 ───

    data class TagData(
        val flatX: Float,
        val flatY: Float,
        val scale: Float,
        val alpha: Float,
        val color: Int,
        /** 正上方投影：影子屏幕X偏移 */
        val shadowFlatX: Float,
        /** 正上方投影：影子屏幕Y偏移 */
        val shadowFlatY: Float,
        /** 正上方投影：影子缩放（越高越大） */
        val shadowScale: Float,
        /** 正上方投影：影子透明度（越高越淡，0表示无影子） */
        val shadowAlpha: Float
    )

    companion object {
        const val FIELDS_PER_TAG = 12

        init {
            System.loadLibrary("tagcloud3d")
        }
    }

    // ─── 缓存 ───

    private var cachedTagData: FloatArray? = null

    // ─── JNI native 方法 ───

    private external fun nativeCreate(): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeSetRadius(handle: Long, radius: Int)
    private external fun nativeSetInertia(handle: Long, inertiaX: Float, inertiaY: Float)
    private external fun nativeSetTagColorDark(handle: Long, colors: FloatArray)
    private external fun nativeSetTagColorLight(handle: Long, colors: FloatArray)
    private external fun nativeSetLightAngle(handle: Long, azimuth: Float, elevation: Float)
    private external fun nativeClear(handle: Long)
    private external fun nativeAddTag(handle: Long, popularity: Int): Int
    private external fun nativeCreateSurfaceDistribution(handle: Long)
    private external fun nativeRecalculateColors(handle: Long)
    private external fun nativeUpdate(handle: Long): Boolean
    private external fun nativeGetTagData(handle: Long): FloatArray?
}
