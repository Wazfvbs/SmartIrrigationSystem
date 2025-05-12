/**
 * @file lv_conf.h
 * Configuration file for v9.3.0-dev
 */

/*
 *将此文件复制为`lv_conf.h`
 *1。仅在`lvgl`文件夹旁边
 *2。或任何其他地方
 * -定义`lv_conf_include_simple`;
 * -将路径添加为包括路径。
 */

/*clang-format off */
#if 1 /*将其设置为“ 1”以启用内容 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*如果您需要在此处包含任何内容，请在`__ semembly__ guard */
#if  0 && defined(__ASSEMBLY__)
#include "my_include.h"
#endif

/*====================
   颜色设置
 *==================*/

/**颜色深度：1（I1），8（L8），16（RGB565），24（RGB888），32（XRGB88888）*/
#define LV_COLOR_DEPTH 16
/*=========================
   stdlib包装器设置
 *=======================*/

/**可能的值
 *-LV_STDLIB_BUILTIN：LVGL内置的实现
 *-LV_STDLIB_CLIB：标准C函数，例如malloc，strlen等
 *-LV_STDLIB_MICROPYTHON：Micropython实施
 * - lv_stdlib_rtthread：rt -thread实现
 * - lv_stdlib_custom：外部实现功能
 */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN

/** Possible values
 * - LV_STDLIB_BUILTIN:     LVGL内置实施
 * - LV_STDLIB_CLIB:        标准C功能，例如malloc，strlen等
 * - LV_STDLIB_MICROPYTHON: Micropython实施
 * - LV_STDLIB_RTTHREAD:    RT线程实现
 * - LV_STDLIB_CUSTOM:      外部实施功能
 */
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN

/** Possible values
 * - LV_STDLIB_BUILTIN:     LVGL's built in implementation
 * - LV_STDLIB_CLIB:        Standard C functions, like malloc, strlen, etc
 * - LV_STDLIB_MICROPYTHON: MicroPython implementation
 * - LV_STDLIB_RTTHREAD:    RT-Thread implementation
 * - LV_STDLIB_CUSTOM:      Implement the functions externally
 */
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
    /**可用于`lv_malloc（）`in Bytes（> = 2kb）可用于的内存大小*/
    #define LV_MEM_SIZE (64 * 1024U)          /**< [bytes] */

   /**存储器的大小扩展为`lv_malloc（）`in Bytes*/
    #define LV_MEM_POOL_EXPAND_SIZE 0

/**为内存池设置一个地址，而不是将其分配为普通数组。也可以在外部SRAM中。 */    
    #define LV_MEM_ADR 0     /**< 0: unused*/
   /*代替地址给出了一个记忆分配器，该内存分配器将被调用以获取LVGL的内存池。例如。 my_malloc */
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif
#endif  /*lv_use_stdlib_malloc == lv_stdlib_builtin*/

/*====================
   HAL设置
 *==================*/

/**默认显示刷新，输入设备读取和动画步骤。 */
#define LV_DEF_REFR_PERIOD  33      /**< [ms] */

/**默认点每英寸。用于初始化默认尺寸，例如小部件大小的样式桨。
 *（并不重要，您可以将其调整以修改默认尺寸和空格。） */
#define LV_DPI_DEF 130              /**< [px/inch] */

/*=================
 *操作系统
 *================*/
/**选择要使用的操作系统。可能的选项：
*-LV_OS_NONE
 *-LV_OS_PTHREAD
 *-LV_OS_FREE
 *-LV_OS_CMSIS_RTOS2
 *-LV_OS_RTTHREAD
 *-LV_OS_WINDOWS
 *-LV_OS_MQX
 *-LV_OS_SDL2
 *-LV_OS_CUSTOM */
#define LV_USE_OS   LV_OS_NONE

#if LV_USE_OS == LV_OS_CUSTOM
    #define LV_OS_CUSTOM_INCLUDE <stdint.h>
#endif
#if LV_USE_OS == LV_OS_FREERTOS
    /*
     *用直接通知解除RTOS任务的速度更快45％，使用较少的RAM
     *比使用中介对象（例如二进制信号量）解开任务。
     *RTOS任务通知只有在只有一个可以成为事件的接收者的任务时才能使用。
     */
    #define LV_USE_FREERTOS_TASK_NOTIFY 1
#endif
/*========================
 *渲染配置
 *======================*/

/**将所有层和图像的大步与此字节相对*/
#define LV_DRAW_BUF_STRIDE_ALIGN                1

/**将draw_buf地址的启动地址对齐此字节*/
#define LV_DRAW_BUF_ALIGN                       4
/**使用矩阵进行转换。
 *要求：
 * -`lv_use_matrix = 1`。
 * -渲染引擎需要支持3x3矩阵变换。 */
#define LV_DRAW_TRANSFORM_USE_MATRIX            0

/* If a widget has `style_opa < 255` (not `bg_opa`, `text_opa` etc) or not NORMAL blend mode
 * it is buffered into a "simple" layer before rendering. The widget can be buffered in smaller chunks.
 * "Transformed layers" (if `transform_angle/zoom` are set) use larger buffers
 * and can't be drawn in chunks. */

/**目标缓冲区大小，用于简单层块。 */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE    (24 * 1024)    /**< [bytes]*/
/*限制简单和转换层的最大分配内存。
 *至少应该是`lv_draw_layer_simple_buf_size`大小
 *也足以存储最大的小部件（宽度x高x 4区域）。
 *将其设置为0，没有限制。 */
#define LV_DRAW_LAYER_MAX_MEMORY 0  /**< No limit by default [bytes]*/
/**绘图线的堆叠尺寸。
 *注意：如果启用了Freetype或Thorvg，建议将其设置为32KB或更多。
 */
#define LV_DRAW_THREAD_STACK_SIZE    (8 * 1024)         /**< [bytes]*/

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
    /*
     *选择性禁用颜色格式支持以减少代码大小。
     *注意：某些功能在内部使用某些颜色格式，例如
     * -梯度使用RGB888
     * -具有透明度的位图可能使用argb8888
     */
    #define LV_DRAW_SW_SUPPORT_RGB565       1
    #define LV_DRAW_SW_SUPPORT_RGB565A8     1
    #define LV_DRAW_SW_SUPPORT_RGB888       1
    #define LV_DRAW_SW_SUPPORT_XRGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED 1
    #define LV_DRAW_SW_SUPPORT_L8           1
    #define LV_DRAW_SW_SUPPORT_AL88         1
    #define LV_DRAW_SW_SUPPORT_A8           1
    #define LV_DRAW_SW_SUPPORT_I1           1

    /*亮度的阈值将像素视为
     *活跃的索引颜色格式 */
    #define LV_DRAW_SW_I1_LUM_THRESHOLD 127
/**设置的抽奖单元数。
     * -> 1要求在`lv_use_os`中启用操作系统。
     * -> 1表示多个线程将并联屏幕。 */
    #define LV_DRAW_SW_DRAW_UNIT_CNT    1
/**使用ARM-2D加速软件（SW）渲染。 */
    #define LV_USE_DRAW_ARM2D_SYNC      0
/**使本地氦气组装得以编译。 */
    #define LV_USE_NATIVE_HELIUM_ASM    0
/**
     *-0：使用能够仅绘制具有渐变，图像，文本和直线的简单矩形的简单渲染器。
     *-1：使用能够绘制圆角，阴影，偏斜线和弧形的复杂渲染器。 */
    #define LV_DRAW_SW_COMPLEX          1

    #if LV_DRAW_SW_COMPLEX == 1/**允许缓冲一些阴影计算。
         *lv_draw_sw_shadow_cache_size是缓冲区的最大阴影大小，阴影大小为
         *`shadow_width + radius“。  缓存具有LV_DRAW_SW_SHADOW_CACHE_SIZE^2 RAM成本。 */
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
/**设置最大圆形数据的数量。
         *1/4圆的圆周保存用于抗降解。
         *`radius *4`字节用于每个圆（最常用的半径是保存的）。
         * - 0：禁用缓存 */
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif

    #define  LV_USE_DRAW_SW_ASM     LV_DRAW_SW_ASM_NONE

    #if LV_USE_DRAW_SW_ASM == LV_DRAW_SW_ASM_CUSTOM
        #define  LV_DRAW_SW_ASM_CUSTOM_INCLUDE ""
    #endif/**在软件中绘制复杂梯度：以角度，径向或圆锥形的线性*/
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS    0

#endif

/*Use TSi's aka (Think Silicon) NemaGFX */
/*使用tsi的又名（思考硅）nemagfx */
#if LV_USE_NEMA_GFX/**选择要使用的Nemagfx HAL。可能的选项：
     *-LV_NEMA_HAL_CUSTOM
     *-LV_NEMA_HAL_STM32 */
    #define LV_USE_NEMA_HAL LV_NEMA_HAL_CUSTOM
    #if LV_USE_NEMA_HAL == LV_NEMA_HAL_STM32
        #define LV_NEMA_STM32_HAL_INCLUDE <stm32u5xx_hal.h>
    #endif

    /*Enable Vector Graphics Operations. Available only if NemaVG library is present*/
    #define LV_USE_NEMA_VG 0
    #if LV_USE_NEMA_VG
        /*Define application's resolution used for VG related buffer allocation */
        #define LV_NEMA_GFX_MAX_RESX 800
        #define LV_NEMA_GFX_MAX_RESY 600
    #endif
#endif
/**在IMX RTXXX平台上使用NXP的VG-Lite GPU。 */
#define LV_USE_DRAW_VGLITE 0

#if LV_USE_DRAW_VGLITE
    /** Enable blit quality degradation workaround recommended for screen's dimension > 352 pixels. */
    #define LV_USE_VGLITE_BLIT_SPLIT 0

    #if LV_USE_OS
        /** Use additional draw thread for VG-Lite processing. */
        #define LV_USE_VGLITE_DRAW_THREAD 1

        #if LV_USE_VGLITE_DRAW_THREAD
            /** Enable VGLite draw async. Queue multiple tasks and flash them once to the GPU. */
            #define LV_USE_VGLITE_DRAW_ASYNC 1
        #endif
    #endif

    /** Enable VGLite asserts. */
    #define LV_USE_VGLITE_ASSERT 0
#endif

/**在IMX RTXXX平台上使用NXP的PXP。 */

#if LV_USE_PXP
    /** Use PXP for drawing.*/
    #define LV_USE_DRAW_PXP 1

    /** Use PXP to rotate display.*/
    #define LV_USE_ROTATE_PXP 0

    #if LV_USE_DRAW_PXP && LV_USE_OS
        /** Use additional draw thread for PXP processing.*/
        #define LV_USE_PXP_DRAW_THREAD 1
    #endif

    /** Enable PXP asserts. */
    #define LV_USE_PXP_ASSERT 0
#endif
/**在MPU平台上使用NXP的G2D。 */
#define LV_USE_DRAW_G2D 0

#if LV_USE_DRAW_G2D
    /** Maximum number of buffers that can be stored for G2D draw unit.
     *  Includes the frame buffers and assets. */
    #define LV_G2D_HASH_TABLE_SIZE 50

    #if LV_USE_OS
        /** Use additional draw thread for G2D processing.*/
        #define LV_USE_G2D_DRAW_THREAD 1
    #endif

    /** Enable G2D asserts. */
    #define LV_USE_G2D_ASSERT 0
#endif

/** Use Renesas Dave2D on RA  platforms. */
#define LV_USE_DRAW_DAVE2D 0

/** Draw using cached SDL textures*/
#define LV_USE_DRAW_SDL 0

/** Use VG-Lite GPU. */
#define LV_USE_DRAW_VG_LITE 0

#if LV_USE_DRAW_VG_LITE
    /** Enable VG-Lite custom external 'gpu_init()' function */
    #define LV_VG_LITE_USE_GPU_INIT 0

    /** Enable VG-Lite assert. */
    #define LV_VG_LITE_USE_ASSERT 0

    /** VG-Lite flush commit trigger threshold. GPU will try to batch these many draw tasks. */
    #define LV_VG_LITE_FLUSH_MAX_COUNT 8

    /** Enable border to simulate shadow.
     *  NOTE: which usually improves performance,
     *  but does not guarantee the same rendering quality as the software. */
    #define LV_VG_LITE_USE_BOX_SHADOW 0

    /** VG-Lite gradient maximum cache number.
     *  @note  The memory usage of a single gradient image is 4K bytes. */
    #define LV_VG_LITE_GRAD_CACHE_CNT 32

    /** VG-Lite stroke maximum cache number. */
    #define LV_VG_LITE_STROKE_CACHE_CNT 32
#endif

/** Accelerate blends, fills, etc. with STM32 DMA2D */
#define LV_USE_DRAW_DMA2D 0

#if LV_USE_DRAW_DMA2D
    #define LV_DRAW_DMA2D_HAL_INCLUDE "stm32h7xx_hal.h"

    /* if enabled, the user is required to call `lv_draw_dma2d_transfer_complete_interrupt_handler`
     * upon receiving the DMA2D global interrupt
     */
    #define LV_USE_DRAW_DMA2D_INTERRUPT 0
#endif

/** Draw using cached OpenGLES textures */
#define LV_USE_DRAW_OPENGLES 0
/*======================
 *功能配置
 *=====================*/

/*--------------------------
 *记录
 *------------*/

/**启用日志模块*/
#define LV_USE_LOG 0
#if LV_USE_LOG/**将值设置为以下记录详细信息之一：
     *-LV_LOG_LEVEL_TRACE日志详细信息。
     *-LV_LOG_LEVEL_INFO日志重要事件。
     *-LV_LOG_LEVEL_WARN日志如果发生了不需要的事情，但没有引起问题。
     *-LV_LOG_LEVEL_ERROR日志仅在系统失败时关键问题。
     *-LV_LOG_LEVEL_USER仅使用用户添加的自定义日志消息。
     * - lv_log_level_none不记录任何内容。 */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /** - 1: Print log with 'printf';
     *  - 0: User needs to register a callback with `lv_log_register_print_cb()`. */
    #define LV_LOG_PRINTF 0

    /** Set callback to print logs.
     *  E.g `my_print`. The prototype should be `void my_print(lv_log_level_t level, const char * buf)`.
     *  Can be overwritten by `lv_log_register_print_cb`. */
    //#define LV_LOG_PRINT_CB

    /** - 1: Enable printing timestamp;
     *  - 0: Disable printing timestamp. */
    #define LV_LOG_USE_TIMESTAMP 1

    /** - 1: Print file and line number of the log;
     *  - 0: Do not print file and line number of the log. */
    #define LV_LOG_USE_FILE_LINE 1

    /* Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs. */
    #define LV_LOG_TRACE_MEM        1   /**< Enable/disable trace logs in memory operations. */
    #define LV_LOG_TRACE_TIMER      1   /**< Enable/disable trace logs in timer operations. */
    #define LV_LOG_TRACE_INDEV      1   /**< Enable/disable trace logs in input device operations. */
    #define LV_LOG_TRACE_DISP_REFR  1   /**< Enable/disable trace logs in display re-draw operations. */
    #define LV_LOG_TRACE_EVENT      1   /**< Enable/disable trace logs in event dispatch logic. */
    #define LV_LOG_TRACE_OBJ_CREATE 1   /**< Enable/disable trace logs in object creation (core `obj` creation plus every widget). */
    #define LV_LOG_TRACE_LAYOUT     1   /**< Enable/disable trace logs in flex- and grid-layout operations. */
    #define LV_LOG_TRACE_ANIM       1   /**< Enable/disable trace logs in animation logic. */
    #define LV_LOG_TRACE_CACHE      1   /**< Enable/disable trace logs in cache operations. */
#endif  /*LV_USE_LOG*/
/*--------------------------
 *断言
 *------------*/

/*如果发现操作失败或发现无效数据，则启用断言失败。
 *如果启用了lv_use_log，则将在失败上打印错误消息。 */
#define LV_USE_ASSERT_NULL          1   /**< Check if the parameter is NULL. (Very fast, recommended) */
#define LV_USE_ASSERT_MALLOC        1   /**< Checks is the memory is successfully allocated or no. (Very fast, recommended) */
#define LV_USE_ASSERT_STYLE         0   /**< Check if the styles are properly initialized. (Very fast, recommended) */
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /**< Check the integrity of `lv_mem` after critical operations. (Slow) */
#define LV_USE_ASSERT_OBJ           0   /**< Check the object's type and existence (e.g. not deleted). (Slow) */

/** Add a custom handler when assert happens e.g. to restart MCU. */
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);     /**< Halt by default */
/*--------------------------
 *调试
 *------------*/

/**1：在重新绘制区域上绘制随机彩色矩形。 */
#define LV_USE_REFR_DEBUG 0

/** 1: Draw a red overlay for ARGB layers and a green overlay for RGB layers*/
#define LV_USE_LAYER_DEBUG 0

/** 1: Adds the following behaviors for debugging:
 *  - Draw overlays with different colors for each draw_unit's tasks.
 *  - Draw index number of draw unit on white background.
 *  - For layers, draws index number of draw unit on black background. */
#define LV_USE_PARALLEL_DRAW_DEBUG 0
/*--------------------------
 *其他的
 *------------*/

#define LV_ENABLE_GLOBAL_CUSTOM 0
#if LV_ENABLE_GLOBAL_CUSTOM
    /** Header to include for custom 'lv_global' function" */
    #define LV_GLOBAL_CUSTOM_INCLUDE <stdint.h>
#endif
/**字节中的默认缓存大小。
 *由图像解码器（例如`lv_lodepng'）使用，以使解码图像保持在内存中。
 *如果未将大小设置为0，则当缓存已满时，解码器将无法解码。
 *如果大小为0，则无法启用缓存函数，并且解码的内存将为
 *使用后立即发布。 */
#define LV_CACHE_DEF_SIZE       0/**图像标头缓存条目的默认号码。缓存用于存储图像的标题
 *主要逻辑就像`lv_cache_def_size`，但对于图像标头。 */
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
/**每个梯度允许的停止数。增加这一点以允许更多停止。
 *这添加了（sizeof（lv_color_t） + 1）每个额外停止的字节。 */
#define LV_GRADIENT_MAX_STOPS   2
/**调整颜色混合功能圆形。 GPU可能以不同的方式计算颜色混合（混合）。
 *-0：下四舍五入，
 *-64：从X.75汇总，
 *-128：从一半开始汇总，
 *-192：从X.25起来，
 *-254：汇总 */
#define LV_COLOR_MIX_ROUND_OFS  0
/**向每个`lv_obj_t`添加2 x 32位变量以加快获取样式属性*/
#define LV_OBJ_STYLE_CACHE      0

/**添加`id`字段到`lv_obj_t`*/

/**启用支持小部件名称*/

/** Automatically assign an ID when obj is created */
#define LV_OBJ_ID_AUTO_ASSIGN   LV_USE_OBJ_ID

/** Use builtin obj ID handler functions:
* - lv_obj_assign_id:       Called when a widget is created. Use a separate counter for each widget class as an ID.
* - lv_obj_id_compare:      Compare the ID to decide if it matches with a requested value.
* - lv_obj_stringify_id:    Return string-ified identifier, e.g. "button3".
* - lv_obj_free_id:         Does nothing, as there is no memory allocation for the ID.
* When disabled these functions needs to be implemented by the user.*/
#define LV_USE_OBJ_ID_BUILTIN   1

/** Use obj property set/get API. */
#define LV_USE_OBJ_PROPERTY 0

/** Enable property name support. */
#define LV_USE_OBJ_PROPERTY_NAME 1

/* Use VG-Lite Simulator.
 * - Requires: LV_USE_THORVG_INTERNAL or LV_USE_THORVG_EXTERNAL */
#define LV_USE_VG_LITE_THORVG  0

#if LV_USE_VG_LITE_THORVG
    /** Enable LVGL's blend mode support */
    #define LV_VG_LITE_THORVG_LVGL_BLEND_SUPPORT 0

    /** Enable YUV color format support */
    #define LV_VG_LITE_THORVG_YUV_SUPPORT 0

    /** Enable Linear gradient extension support */
    #define LV_VG_LITE_THORVG_LINEAR_GRADIENT_EXT_SUPPORT 0

    /** Enable alignment on 16 pixels */
    #define LV_VG_LITE_THORVG_16PIXELS_ALIGN 1

    /** Buffer address alignment */
    #define LV_VG_LITE_THORVG_BUF_ADDR_ALIGN 64

    /** Enable multi-thread render */
    #define LV_VG_LITE_THORVG_THREAD_RENDER 0
#endif

/* Enable the multi-touch gesture recognition feature */
/* Gesture recognition requires the use of floats */
#define LV_USE_GESTURE_RECOGNITION 0

/*=====================
 *  COMPILER SETTINGS
 *====================*/

/** For big endian systems set to 1 */
#define LV_BIG_ENDIAN_SYSTEM 0

/** Define a custom attribute for `lv_tick_inc` function */
#define LV_ATTRIBUTE_TICK_INC

/** Define a custom attribute for `lv_timer_handler` function */
#define LV_ATTRIBUTE_TIMER_HANDLER

/** Define a custom attribute for `lv_display_flush_ready` function */
#define LV_ATTRIBUTE_FLUSH_READY

/** Align VG_LITE buffers on this number of bytes.
 *  @note  vglite_src_buf_aligned() uses this value to validate alignment of passed buffer pointers. */
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/** Will be added where memory needs to be aligned (with -Os data might not be aligned to boundary by default).
 *  E.g. __attribute__((aligned(4)))*/
#define LV_ATTRIBUTE_MEM_ALIGN

/** Attribute to mark large constant arrays, for example for font bitmaps */
#define LV_ATTRIBUTE_LARGE_CONST

/** Compiler prefix for a large array declaration in RAM */
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/** Place performance critical functions into a faster memory (e.g RAM) */
#define LV_ATTRIBUTE_FAST_MEM

/** Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that
 *  should also appear on LVGL binding API such as MicroPython. */
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning  /**< The default value just prevents GCC warning */

/** Prefix all global extern data with this */
#define LV_ATTRIBUTE_EXTERN_DATA

/** Use `float` as `lv_value_precise_t` */
#define LV_USE_FLOAT            0

/** Enable matrix support
 *  - Requires `LV_USE_FLOAT = 1` */
#define LV_USE_MATRIX           0

/** Include `lvgl_private.h` in `lvgl.h` to access internal data and functions by default */
#ifndef LV_USE_PRIVATE_API
    #define LV_USE_PRIVATE_API  0
#endif
/*====================
 *使用
 *=================== //

/* Montserrat fonts with ASCII range and some symbols using bpp = 4
 * https://fonts.google.com/specimen/Montserrat */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_28_COMPRESSED    0  /**< bpp = 3 */
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0  /**< Hebrew, Arabic, Persian letters and all their forms */
#define LV_FONT_SIMSUN_14_CJK               0  /**< 1000 most common CJK radicals */
#define LV_FONT_SIMSUN_16_CJK               0  /**< 1000 most common CJK radicals */
#define LV_FONT_SOURCE_HAN_SANS_SC_14_CJK   0  /**< 1338 most common CJK radicals */
#define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK   0  /**< 1338 most common CJK radicals */

/** Pixel perfect monospaced fonts */
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/** Optionally declare custom fonts here.
 *
 *  You can use any of these fonts as the default font too and they will be available
 *  globally.  Example:
 *
 *  @code
 *  #define LV_FONT_CUSTOM_DECLARE   LV_FONT_DECLARE(my_font_1) LV_FONT_DECLARE(my_font_2)
 *  @endcode
 */
#define LV_FONT_CUSTOM_DECLARE

/** Always set a default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/** Enable handling large font and/or fonts with a lot of characters.
 *  The limit depends on the font size, font face and bpp.
 *  A compiler error will be triggered if a font needs it. */
#define LV_FONT_FMT_TXT_LARGE 0

/** Enables/disables support for compressed fonts. */
#define LV_USE_FONT_COMPRESSED 0

/** Enable drawing placeholders when glyph dsc is not found. */
#define LV_USE_FONT_PLACEHOLDER 1
/*=================
 *文本设置
 *================*/

/**
 * Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding.
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/** While rendering text strings, break (wrap) text on these chars. */
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"

/** If a word is at least this long, will break wherever "prettiest".
 *  To disable, set to a value <= 0. */
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/** Minimum number of characters in a long word to put on a line before a break.
 *  Depends on LV_TXT_LINE_BREAK_LONG_LEN. */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/** Minimum number of characters in a long word to put on a line after a break.
 *  Depends on LV_TXT_LINE_BREAK_LONG_LEN. */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/** Support bidirectional text. Allows mixing Left-to-Right and Right-to-Left text.
 *  The direction will be processed according to the Unicode Bidirectional Algorithm:
 *  https://www.w3.org/International/articles/inline-bidi-markup/uba-basics */
#define LV_USE_BIDI 0
#if LV_USE_BIDI
    /*Set the default direction. Supported values:
    *`LV_BASE_DIR_LTR` Left-to-Right
    *`LV_BASE_DIR_RTL` Right-to-Left
    *`LV_BASE_DIR_AUTO` detect text base direction*/
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/** Enable Arabic/Persian processing
 *  In these languages characters should be replaced with another form based on their position in the text */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*The control character to use for signaling text recoloring*/
#define LV_TXT_COLOR_CMD "#"

/*==================
 * WIDGETS
 *================*/
/* Documentation for widgets can be found here: https://docs.lvgl.io/master/details/widgets/index.html . */

/** 1: Causes these widgets to be given default values at creation time.
 *  - lv_buttonmatrix_t:  Get default maps:  {"Btn1", "Btn2", "Btn3", "\n", "Btn4", "Btn5", ""}, else map not set.
 *  - lv_checkbox_t    :  String label set to "Check box", else set to empty string.
 *  - lv_dropdown_t    :  Options set to "Option 1", "Option 2", "Option 3", else no values are set.
 *  - lv_roller_t      :  Options set to "Option 1", "Option 2", "Option 3", "Option 4", "Option 5", else no values are set.
 *  - lv_label_t       :  Text set to "Text", else empty string.
 * */
#define LV_WIDGETS_HAS_DEFAULT_VALUE  1

#define LV_USE_ANIMIMG    1

#define LV_USE_ARC        1

#define LV_USE_BAR        1

#define LV_USE_BUTTON        1

#define LV_USE_BUTTONMATRIX  1

#define LV_USE_CALENDAR   1
#if LV_USE_CALENDAR
    #define LV_CALENDAR_WEEK_STARTS_MONDAY 0
    #if LV_CALENDAR_WEEK_STARTS_MONDAY
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
    #else
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
    #endif

    #define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}
    #define LV_USE_CALENDAR_HEADER_ARROW 1
    #define LV_USE_CALENDAR_HEADER_DROPDOWN 1
    #define LV_USE_CALENDAR_CHINESE 0
#endif  /*LV_USE_CALENDAR*/

#define LV_USE_CANVAS     1

#define LV_USE_CHART      1

#define LV_USE_CHECKBOX   1

#define LV_USE_DROPDOWN   1   /**< Requires: lv_label */

#define LV_USE_IMAGE      1   /**< Requires: lv_label */

#define LV_USE_IMAGEBUTTON     1

#define LV_USE_KEYBOARD   1

#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1   /**< Enable selecting text of the label */
    #define LV_LABEL_LONG_TXT_HINT 1    /**< Store some extra info in labels to speed up drawing of very long text */
    #define LV_LABEL_WAIT_CHAR_COUNT 3  /**< The count of wait chart */
#endif

#define LV_USE_LED        1

#define LV_USE_LINE       1

#define LV_USE_LIST       1

#define LV_USE_LOTTIE     0  /**< Requires: lv_canvas, thorvg */

#define LV_USE_MENU       1

#define LV_USE_MSGBOX     1

#define LV_USE_ROLLER     1   /**< Requires: lv_label */

#define LV_USE_SCALE      1

#define LV_USE_SLIDER     1   /**< Requires: lv_bar */

#define LV_USE_SPAN       1
#if LV_USE_SPAN
    /** A line of text can contain this maximum number of span descriptors. */
    #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

#define LV_USE_SPINBOX    1

#define LV_USE_SPINNER    1

#define LV_USE_SWITCH     1

#define LV_USE_TABLE      1

#define LV_USE_TABVIEW    1

#define LV_USE_TEXTAREA   1   /**< Requires: lv_label */
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500    /**< [ms] */
#endif

#define LV_USE_TILEVIEW   1

#define LV_USE_WIN        1

#define LV_USE_3DTEXTURE  0

/*==================
 * THEMES
 *==================*/
/* Documentation for themes can be found here: https://docs.lvgl.io/master/details/common-widget-features/styles/style.html#themes . */

/** A simple, impressive and very complete theme */
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    /** 0: Light mode; 1: Dark mode */
    #define LV_THEME_DEFAULT_DARK 0

    /** 1: Enable grow on press */
    #define LV_THEME_DEFAULT_GROW 1

    /** Default transition time in ms. */
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif /*LV_USE_THEME_DEFAULT*/

/** A very simple theme that is a good starting point for a custom theme */
#define LV_USE_THEME_SIMPLE 1

/** A theme designed for monochrome displays */
#define LV_USE_THEME_MONO 1

/*==================
 * LAYOUTS
 *==================*/
/* Documentation for layouts can be found here: https://docs.lvgl.io/master/details/common-widget-features/layouts/index.html . */

/** A layout similar to Flexbox in CSS. */
#define LV_USE_FLEX 1

/** A layout similar to Grid in CSS. */
#define LV_USE_GRID 1

/*====================
 * 3RD PARTS LIBRARIES
 *====================*/
/* Documentation for libraries can be found here: https://docs.lvgl.io/master/details/libs/index.html . */

/* File system interfaces for common APIs */

/** Setting a default driver letter allows skipping the driver prefix in filepaths.
 *  Documentation about how to use the below driver-identifier letters can be found at
 *  https://docs.lvgl.io/master/details/main-modules/fs.html#lv-fs-identifier-letters . */
#define LV_FS_DEFAULT_DRIVER_LETTER '\0'

/** API for fopen, fread, etc. */
#define LV_USE_FS_STDIO 0
#if LV_USE_FS_STDIO
    #define LV_FS_STDIO_LETTER '\0'     /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
    #define LV_FS_STDIO_PATH ""         /**< Set the working directory. File/directory paths will be appended to it. */
    #define LV_FS_STDIO_CACHE_SIZE 0    /**< >0 to cache this number of bytes in lv_fs_read() */
#endif

/** API for open, read, etc. */
#define LV_USE_FS_POSIX 0
#if LV_USE_FS_POSIX
    #define LV_FS_POSIX_LETTER '\0'     /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
    #define LV_FS_POSIX_PATH ""         /**< Set the working directory. File/directory paths will be appended to it. */
    #define LV_FS_POSIX_CACHE_SIZE 0    /**< >0 to cache this number of bytes in lv_fs_read() */
#endif

/** API for CreateFile, ReadFile, etc. */
#define LV_USE_FS_WIN32 0
#if LV_USE_FS_WIN32
    #define LV_FS_WIN32_LETTER '\0'     /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
    #define LV_FS_WIN32_PATH ""         /**< Set the working directory. File/directory paths will be appended to it. */
    #define LV_FS_WIN32_CACHE_SIZE 0    /**< >0 to cache this number of bytes in lv_fs_read() */
#endif

/** API for FATFS (needs to be added separately). Uses f_open, f_read, etc. */
#define LV_USE_FS_FATFS 0
#if LV_USE_FS_FATFS
    #define LV_FS_FATFS_LETTER '\0'     /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
    #define LV_FS_FATFS_PATH ""         /**< Set the working directory. File/directory paths will be appended to it. */
    #define LV_FS_FATFS_CACHE_SIZE 0    /**< >0 to cache this number of bytes in lv_fs_read() */
#endif

/** API for memory-mapped file access. */
#define LV_USE_FS_MEMFS 0
#if LV_USE_FS_MEMFS
    #define LV_FS_MEMFS_LETTER '\0'     /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
#endif

/** API for LittleFs. */
#define LV_USE_FS_LITTLEFS 0
#if LV_USE_FS_LITTLEFS
    #define LV_FS_LITTLEFS_LETTER '\0'  /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
    #define LV_FS_LITTLEFS_PATH ""      /**< Set the working directory. File/directory paths will be appended to it. */
#endif

/** API for Arduino LittleFs. */
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#if LV_USE_FS_ARDUINO_ESP_LITTLEFS
    #define LV_FS_ARDUINO_ESP_LITTLEFS_LETTER '\0'  /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
    #define LV_FS_ARDUINO_ESP_LITTLEFS_PATH ""      /**< Set the working directory. File/directory paths will be appended to it. */
#endif

/** API for Arduino Sd. */
#define LV_USE_FS_ARDUINO_SD 0
#if LV_USE_FS_ARDUINO_SD
    #define LV_FS_ARDUINO_SD_LETTER '\0'  /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
    #define LV_FS_ARDUINO_SD_PATH ""      /**< Set the working directory. File/directory paths will be appended to it. */
#endif

/** API for UEFI */
#define LV_USE_FS_UEFI 0
#if LV_USE_FS_UEFI
    #define LV_FS_UEFI_LETTER '\0'      /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
#endif

/** LODEPNG decoder library */
#define LV_USE_LODEPNG 0

/** PNG decoder(libpng) library */
#define LV_USE_LIBPNG 0

/** BMP decoder library */
#define LV_USE_BMP 0

/** JPG + split JPG decoder library.
 *  Split JPG is a custom format optimized for embedded systems. */
#define LV_USE_TJPGD 0

/** libjpeg-turbo decoder library.
 *  - Supports complete JPEG specifications and high-performance JPEG decoding. */
#define LV_USE_LIBJPEG_TURBO 0

/** GIF decoder library */
#define LV_USE_GIF 0
#if LV_USE_GIF
    /** GIF decoder accelerate */
    #define LV_GIF_CACHE_DECODE_DATA 0
#endif


/** Decode bin images to RAM */
#define LV_BIN_DECODER_RAM_LOAD 0

/** RLE decompress library */
#define LV_USE_RLE 0

/** QR code library */
#define LV_USE_QRCODE 0

/** Barcode code library */
#define LV_USE_BARCODE 0

/** FreeType library */
#define LV_USE_FREETYPE 0
#if LV_USE_FREETYPE
    /** Let FreeType use LVGL memory and file porting */
    #define LV_FREETYPE_USE_LVGL_PORT 0

    /** Cache count of glyphs in FreeType, i.e. number of glyphs that can be cached.
     *  The higher the value, the more memory will be used. */
    #define LV_FREETYPE_CACHE_FT_GLYPH_CNT 256
#endif

/** Built-in TTF decoder */
#define LV_USE_TINY_TTF 0
#if LV_USE_TINY_TTF
    /* Enable loading TTF data from files */
    #define LV_TINY_TTF_FILE_SUPPORT 0
    #define LV_TINY_TTF_CACHE_GLYPH_CNT 256
#endif

/** Rlottie library */
#define LV_USE_RLOTTIE 0

/** Enable Vector Graphic APIs
 *  - Requires `LV_USE_MATRIX = 1` */
#define LV_USE_VECTOR_GRAPHIC  0

/** Enable ThorVG (vector graphics library) from the src/libs folder */
#define LV_USE_THORVG_INTERNAL 0

/** Enable ThorVG by assuming that its installed and linked to the project */
#define LV_USE_THORVG_EXTERNAL 0

/** Use lvgl built-in LZ4 lib */
#define LV_USE_LZ4_INTERNAL  0

/** Use external LZ4 library */
#define LV_USE_LZ4_EXTERNAL  0

/*SVG library
 *  - Requires `LV_USE_VECTOR_GRAPHIC = 1` */
#define LV_USE_SVG 0
#define LV_USE_SVG_ANIMATION 0
#define LV_USE_SVG_DEBUG 0

/** FFmpeg library for image decoding and playing videos.
 *  Supports all major image formats so do not enable other image decoder with it. */
#define LV_USE_FFMPEG 0
#if LV_USE_FFMPEG
    /** Dump input information to stderr */
    #define LV_FFMPEG_DUMP_FORMAT 0
    /** Use lvgl file path in FFmpeg Player widget
     *  You won't be able to open URLs after enabling this feature.
     *  Note that FFmpeg image decoder will always use lvgl file system. */
    #define LV_FFMPEG_PLAYER_USE_LV_FS 0
#endif

/*==================
 * OTHERS
 *==================*/
/* Documentation for several of the below items can be found here: https://docs.lvgl.io/master/details/auxiliary-modules/index.html . */

/** 1: Enable API to take snapshot for object */
#define LV_USE_SNAPSHOT 0

/** 1: Enable system monitor component */
#define LV_USE_SYSMON   0
#if LV_USE_SYSMON
    /** Get the idle percentage. E.g. uint32_t my_get_idle(void); */
    #define LV_SYSMON_GET_IDLE lv_os_get_idle_percent

    /** 1: Show CPU usage and FPS count.
     *  - Requires `LV_USE_SYSMON = 1` */
    #define LV_USE_PERF_MONITOR 0
    #if LV_USE_PERF_MONITOR
        #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT

        /** 0: Displays performance data on the screen; 1: Prints performance data using log. */
        #define LV_USE_PERF_MONITOR_LOG_MODE 0
    #endif

    /** 1: Show used memory and memory fragmentation.
     *     - Requires `LV_USE_STDLIB_MALLOC = LV_STDLIB_BUILTIN`
     *     - Requires `LV_USE_SYSMON = 1`*/
    #define LV_USE_MEM_MONITOR 0
    #if LV_USE_MEM_MONITOR
        #define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT
    #endif
#endif /*LV_USE_SYSMON*/

/** 1: Enable runtime performance profiler */
#define LV_USE_PROFILER 0
#if LV_USE_PROFILER
    /** 1: Enable the built-in profiler */
    #define LV_USE_PROFILER_BUILTIN 1
    #if LV_USE_PROFILER_BUILTIN
        /** Default profiler trace buffer size */
        #define LV_PROFILER_BUILTIN_BUF_SIZE (16 * 1024)     /**< [bytes] */
        #define LV_PROFILER_BUILTIN_DEFAULT_ENABLE 1
    #endif

    /** Header to include for profiler */
    #define LV_PROFILER_INCLUDE "lvgl/src/misc/lv_profiler_builtin.h"

    /** Profiler start point function */
    #define LV_PROFILER_BEGIN    LV_PROFILER_BUILTIN_BEGIN

    /** Profiler end point function */
    #define LV_PROFILER_END      LV_PROFILER_BUILTIN_END

    /** Profiler start point function with custom tag */
    #define LV_PROFILER_BEGIN_TAG LV_PROFILER_BUILTIN_BEGIN_TAG

    /** Profiler end point function with custom tag */
    #define LV_PROFILER_END_TAG   LV_PROFILER_BUILTIN_END_TAG

    /*Enable layout profiler*/
    #define LV_PROFILER_LAYOUT 1

    /*Enable disp refr profiler*/
    #define LV_PROFILER_REFR 1

    /*Enable draw profiler*/
    #define LV_PROFILER_DRAW 1

    /*Enable indev profiler*/
    #define LV_PROFILER_INDEV 1

    /*Enable decoder profiler*/
    #define LV_PROFILER_DECODER 1

    /*Enable font profiler*/
    #define LV_PROFILER_FONT 1

    /*Enable fs profiler*/
    #define LV_PROFILER_FS 1

    /*Enable style profiler*/
    #define LV_PROFILER_STYLE 0

    /*Enable timer profiler*/
    #define LV_PROFILER_TIMER 1

    /*Enable cache profiler*/
    #define LV_PROFILER_CACHE 1

    /*Enable event profiler*/
    #define LV_PROFILER_EVENT 1
#endif

/** 1: Enable Monkey test */
#define LV_USE_MONKEY 0

/** 1: Enable grid navigation */
#define LV_USE_GRIDNAV 0

/** 1: Enable `lv_obj` fragment logic */
#define LV_USE_FRAGMENT 0

/** 1: Support using images as font in label or span widgets */
#define LV_USE_IMGFONT 0

/** 1: Enable an observer pattern implementation */
#define LV_USE_OBSERVER 1

/** 1: Enable Pinyin input method
 *  - Requires: lv_keyboard */
#define LV_USE_IME_PINYIN 0
#if LV_USE_IME_PINYIN
    /** 1: Use default thesaurus.
     *  @note  If you do not use the default thesaurus, be sure to use `lv_ime_pinyin` after setting the thesaurus. */
    #define LV_IME_PINYIN_USE_DEFAULT_DICT 1
    /** Set maximum number of candidate panels that can be displayed.
     *  @note  This needs to be adjusted according to size of screen. */
    #define LV_IME_PINYIN_CAND_TEXT_NUM 6

    /** Use 9-key input (k9). */
    #define LV_IME_PINYIN_USE_K9_MODE      1
    #if LV_IME_PINYIN_USE_K9_MODE == 1
        #define LV_IME_PINYIN_K9_CAND_TEXT_NUM 3
    #endif /*LV_IME_PINYIN_USE_K9_MODE*/
#endif

/** 1: Enable file explorer.
 *  - Requires: lv_table */
#define LV_USE_FILE_EXPLORER                     0
#if LV_USE_FILE_EXPLORER
    /** Maximum length of path */
    #define LV_FILE_EXPLORER_PATH_MAX_LEN        (128)
    /** Quick access bar, 1:use, 0:do not use.
     *  - Requires: lv_list */
    #define LV_FILE_EXPLORER_QUICK_ACCESS        1
#endif

/** 1: Enable Font manager */
#define LV_USE_FONT_MANAGER                     0
#if LV_USE_FONT_MANAGER

/**Font manager name max length*/
#define LV_FONT_MANAGER_NAME_MAX_LEN            32

#endif

/** Enable emulated input devices, time emulation, and screenshot compares. */
#define LV_USE_TEST 0
#if LV_USE_TEST

/** Enable `lv_test_screenshot_compare`.
 * Requires libpng and a few MB of extra RAM. */
#define LV_USE_TEST_SCREENSHOT_COMPARE 0
#endif /*LV_USE_TEST*/

/** Enable loading XML UIs runtime */
#define LV_USE_XML    0

/*1: Enable color filter style*/
#define LV_USE_COLOR_FILTER     0
/*==================
 * DEVICES
 *==================*/

/** Use SDL to open window on PC and handle mouse and keyboard. */
#define LV_USE_SDL              0
#if LV_USE_SDL
    #define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h>
    #define LV_SDL_RENDER_MODE      LV_DISPLAY_RENDER_MODE_DIRECT   /**< LV_DISPLAY_RENDER_MODE_DIRECT is recommended for best performance */
    #define LV_SDL_BUF_COUNT        1    /**< 1 or 2 */
    #define LV_SDL_ACCELERATED      1    /**< 1: Use hardware acceleration*/
    #define LV_SDL_FULLSCREEN       0    /**< 1: Make the window full screen by default */
    #define LV_SDL_DIRECT_EXIT      1    /**< 1: Exit the application when all SDL windows are closed */
    #define LV_SDL_MOUSEWHEEL_MODE  LV_SDL_MOUSEWHEEL_MODE_ENCODER  /*LV_SDL_MOUSEWHEEL_MODE_ENCODER/CROWN*/
#endif

/** Use X11 to open window on Linux desktop and handle mouse and keyboard */
#define LV_USE_X11              0
#if LV_USE_X11
    #define LV_X11_DIRECT_EXIT         1  /**< Exit application when all X11 windows have been closed */
    #define LV_X11_DOUBLE_BUFFER       1  /**< Use double buffers for rendering */
    /* Select only 1 of the following render modes (LV_X11_RENDER_MODE_PARTIAL preferred!). */
    #define LV_X11_RENDER_MODE_PARTIAL 1  /**< Partial render mode (preferred) */
    #define LV_X11_RENDER_MODE_DIRECT  0  /**< Direct render mode */
    #define LV_X11_RENDER_MODE_FULL    0  /**< Full render mode */
#endif

/** Use Wayland to open a window and handle input on Linux or BSD desktops */
#define LV_USE_WAYLAND          0
#if LV_USE_WAYLAND
    #define LV_WAYLAND_WINDOW_DECORATIONS   0    /**< Draw client side window decorations only necessary on Mutter/GNOME */
    #define LV_WAYLAND_WL_SHELL             0    /**< Use the legacy wl_shell protocol instead of the default XDG shell */
#endif

/** Driver for /dev/fb */
#define LV_USE_LINUX_FBDEV      0
#if LV_USE_LINUX_FBDEV
    #define LV_LINUX_FBDEV_BSD           0
    #define LV_LINUX_FBDEV_RENDER_MODE   LV_DISPLAY_RENDER_MODE_PARTIAL
    #define LV_LINUX_FBDEV_BUFFER_COUNT  0
    #define LV_LINUX_FBDEV_BUFFER_SIZE   60
    #define LV_LINUX_FBDEV_MMAP          1
#endif

/** Use Nuttx to open window and handle touchscreen */
#define LV_USE_NUTTX    0

#if LV_USE_NUTTX
    #define LV_USE_NUTTX_INDEPENDENT_IMAGE_HEAP 0

    /** Use independent image heap for default draw buffer */
    #define LV_NUTTX_DEFAULT_DRAW_BUF_USE_INDEPENDENT_IMAGE_HEAP    0

    #define LV_USE_NUTTX_LIBUV    0

    /** Use Nuttx custom init API to open window and handle touchscreen */
    #define LV_USE_NUTTX_CUSTOM_INIT    0

    /** Driver for /dev/lcd */
    #define LV_USE_NUTTX_LCD      0
    #if LV_USE_NUTTX_LCD
        #define LV_NUTTX_LCD_BUFFER_COUNT    0
        #define LV_NUTTX_LCD_BUFFER_SIZE     60
    #endif

    /** Driver for /dev/input */
    #define LV_USE_NUTTX_TOUCHSCREEN    0

    /*Touchscreen cursor size in pixels(<=0: disable cursor)*/
    #define LV_NUTTX_TOUCHSCREEN_CURSOR_SIZE    0
#endif

/** Driver for /dev/dri/card */
#define LV_USE_LINUX_DRM        0

#if LV_USE_LINUX_DRM

    /* Use the MESA GBM library to allocate DMA buffers that can be
     * shared across sub-systems and libraries using the Linux DMA-BUF API.
     * The GBM library aims to provide a platform independent memory management system
     * it supports the major GPU vendors - This option requires linking with libgbm */
    #define LV_LINUX_DRM_GBM_BUFFERS 0
#endif

/** Interface for TFT_eSPI */
#define LV_USE_TFT_ESPI         0

/** Driver for evdev input devices */
#define LV_USE_EVDEV    0

/** Driver for libinput input devices */
#define LV_USE_LIBINPUT    0

#if LV_USE_LIBINPUT
    #define LV_LIBINPUT_BSD    0

    /** Full keyboard support */
    #define LV_LIBINPUT_XKB             0
    #if LV_LIBINPUT_XKB
        /** "setxkbmap -query" can help find the right values for your keyboard */
        #define LV_LIBINPUT_XKB_KEY_MAP { .rules = NULL, .model = "pc101", .layout = "us", .variant = NULL, .options = NULL }
    #endif
#endif

/* Drivers for LCD devices connected via SPI/parallel port */
#define LV_USE_ST7735        0
#define LV_USE_ST7789        0
#define LV_USE_ST7796        1
#define LV_USE_ILI9341       0
#define LV_USE_FT81X         0

#if (LV_USE_ST7735 | LV_USE_ST7789 | LV_USE_ST7796 | LV_USE_ILI9341)
    #define LV_USE_GENERIC_MIPI 1
#else
    #define LV_USE_GENERIC_MIPI 0
#endif

/** Driver for Renesas GLCD */
#define LV_USE_RENESAS_GLCDC    0

/** Driver for ST LTDC */
#define LV_USE_ST_LTDC    0
#if LV_USE_ST_LTDC
    /* Only used for partial. */
    #define LV_ST_LTDC_USE_DMA2D_FLUSH 0
#endif

/** LVGL Windows backend */
#define LV_USE_WINDOWS    0

/** LVGL UEFI backend */
#define LV_USE_UEFI 0
#if LV_USE_UEFI
    #define LV_USE_UEFI_INCLUDE "myefi.h"   /**< Header that hides the actual framework (EDK2, gnu-efi, ...) */
    #define LV_UEFI_USE_MEMORY_SERVICES 0   /**< Use the memory functions from the boot services table */
#endif

/** Use OpenGL to open window on PC and handle mouse and keyboard */
#define LV_USE_OPENGLES   0
#if LV_USE_OPENGLES
    #define LV_USE_OPENGLES_DEBUG        1    /**< Enable or disable debug for opengles */
#endif

/** QNX Screen display and input drivers */
#define LV_USE_QNX              0
#if LV_USE_QNX
    #define LV_QNX_BUF_COUNT        1    /**< 1 or 2 */
#endif

/*==================
* EXAMPLES
*==================*/

/** Enable examples to be built with the library. */
#define LV_BUILD_EXAMPLES 1

/*===================
 * DEMO USAGE
 ====================*/

/** Show some widgets. This might be required to increase `LV_MEM_SIZE`. */
#define LV_USE_DEMO_WIDGETS 0

/** Demonstrate usage of encoder and keyboard. */
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

/** Benchmark your system */
#define LV_USE_DEMO_BENCHMARK 0

/** Render test for each primitive.
 *  - Requires at least 480x272 display. */
#define LV_USE_DEMO_RENDER 0

/** Stress test for LVGL */
#define LV_USE_DEMO_STRESS 0

/** Music player demo */
#define LV_USE_DEMO_MUSIC 0
#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_SQUARE    0
    #define LV_DEMO_MUSIC_LANDSCAPE 0
    #define LV_DEMO_MUSIC_ROUND     0
    #define LV_DEMO_MUSIC_LARGE     0
    #define LV_DEMO_MUSIC_AUTO_PLAY 0
#endif

/** Vector graphic demo */
#define LV_USE_DEMO_VECTOR_GRAPHIC  0

/*---------------------------
 * Demos from lvgl/lv_demos
  ---------------------------*/

/** Flex layout demo */
#define LV_USE_DEMO_FLEX_LAYOUT     0

/** Smart-phone like multi-language demo */
#define LV_USE_DEMO_MULTILANG       0

/** Widget transformation demo */
#define LV_USE_DEMO_TRANSFORM       0

/** Demonstrate scroll settings */
#define LV_USE_DEMO_SCROLL          0

/*E-bike demo with Lottie animations (if LV_USE_LOTTIE is enabled)*/
#define LV_USE_DEMO_EBIKE           0
#if LV_USE_DEMO_EBIKE
    #define LV_DEMO_EBIKE_PORTRAIT  0    /*0: for 480x270..480x320, 1: for 480x800..720x1280*/
#endif

/** High-resolution demo */
#define LV_USE_DEMO_HIGH_RES        0

/* Smart watch demo */
#define LV_USE_DEMO_SMARTWATCH      0

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/
