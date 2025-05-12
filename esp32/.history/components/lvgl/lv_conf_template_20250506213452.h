/**
 * @file lv_conf.h
 *v8.3.11的配置文件
 */

/*
 *将此文件复制为`lv_conf.h`
 *1。只是在`lvgl`文件夹旁边
 *2。或其他任何地方
 * -定义`lv_conf_include_simple`
 * -添加路径为包括路径
 */

/* 叮当响 */
#if 0 /*将其设置为“ 1”以启用内容*/

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*===================
   颜色设置
 *==================*/

/*颜色深度：1（每像素1字节），8（RGB332），16（RGB565），32（argb88888）*/
#define LV_COLOR_DEPTH 16

/*交换RGB565颜色的2个字节。如果显示具有8位接口（例如SPI），则有用*/
#define LV_COLOR_16_SWAP 0

/*启用功能可以借鉴透明背景。
 *如果使用OPA和Transform_*使用样式属性，则需要。
 *C如果UI在另一层上方，例如OSD菜单或视频播放器。*/
#define LV_COLOR_SCREEN_TRANSP 0

/* 调整颜色混合功能舍入。 GPU可能以不同的方式计算颜色混合（混合）。
 * 0：圆下64：从X.75，128：从半半起，从192：从X.25，254汇总：圆形 */
#define LV_COLOR_MIX_ROUND_OFS 0

/*如果具有色度的图像像素，则不会绘制它们的色度键合）*/
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)         /*纯绿色*/

/*=======================
   内存设置
 *======================*/

/*1：使用自定义malloc/free，0：使用内置`lv_mem_alloc（）`and'lv_mem_free（）``*/
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    /*可用于`lv_mem_alloc（）`in Bytes（> = 2kb）的内存大小*/
    #define LV_MEM_SIZE (48U * 1024U)          /*[字节]*/

    /*为内存池设置一个地址，而不是将其分配为普通数组。也可以在外部SRAM中。*/
    #define LV_MEM_ADR 0     /*0：未使用*/
    /*代替地址给出了一个记忆分配器，该分配器将被调用以获取LVGL的内存池。例如。 my_malloc*/
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif

#else       /*LV MEM定制*/
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*动态内存功能的标题*/
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /*LV MEM定制*/

/*在渲染和其他内部处理机制中使用的中间存储缓冲区的数量。
 *Y如果没有足够的缓冲区，您将看到错误日志消息。 */
#define LV_MEM_BUF_MAX_NUM 16

/*使用标准的“ memcpy”和“ memset”，而不是LVGL自己的功能。 （可能不会更快或不会更快）。*/
#define LV_MEMCPY_MEMSET_STD 0

/*===================
   HAL设置
 *==================*/

/*默认显示刷新期。 LVG将在此期间重新修改区域*/
#define LV_DISP_DEF_REFR_PERIOD 30      /*[多发性硬化症]*/

/*输入设备以毫秒为单位的读取期*/
#define LV_INDEV_DEF_READ_PERIOD 30     /*[多发性硬化症]*/

/*使用一个自定义的刻度源，该来源告诉毫秒以毫秒为单位。
 *It消除了用`lv_tick_inc（）`）手动更新tick的需求*/
#define LV_TICK_CUSTOM 0
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         /*系统时间功能的标题*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /*在MS中评估当前系统时间的表达式*/
    /*如果使用LVGL为ESP32组件*/
#define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
#define lv_tick_custom_sys_time_expr((ESP_TIMER_GET_TIME()) / 1000LL)
#endif   /*lv tick习惯*/

/*默认点每英寸。用于初始化默认尺寸，例如小部件大小的样式桨。
 *(并不重要，您可以将其调整以修改默认尺寸和空间）*/
#define LV_DPI_DEF 130     /*[PX/英寸]*/

/*=====================
 *功能配置
 *====================*/

/*-------------
 *绘画
 *-----------*/

/*启用复杂的拉动引擎。
 *R需要画阴影，渐变，圆角，圆圈，弧线，偏斜线，图像转换或任何口罩*/
#define LV_DRAW_COMPLEX 1
#if LV_DRAW_COMPLEX != 0

    /*允许缓冲一些阴影计算。
    *lv_shadow_cache_size是最大。阴影大小到缓冲区，那里的阴影大小为`shadow_width + radius'
    *CAching具有LV_SHADOW_CACHE_SIZE^2 RAM成本*/
    #define LV_SHADOW_CACHE_SIZE 0

    /* 设置最大缓存圆圈数据的数量。
    *1/4圈的圆周保存用于抗降解
    *半径 *4个字节是每个圆的（保存最常用的半径）
    * 0：禁用缓存 */
    #define LV_CIRCLE_CACHE_SIZE 4
#endif /*LV绘制复合物*/

/**
 *当小部件具有'style_opa <255`将小部件缓冲到一层时，使用“简单层”
 *并将其作为图像与给定的不透明度混合。
 *请注意，`bg_opa`，`text_opa`等不需要对层进行缓冲）
 *小部件可以在较小的块中进行缓冲，以避免使用大型缓冲区。
 *
 *-lv_layer_simple_buf_size：[bytes]最佳目标缓冲区大小。 LVGL将尝试分配它
 *-lv_layer_simple_fallback_buf_size：[bytes]使用`lv_layer_simple_buf_size'无法分配。
 *
 *两个缓冲尺寸都在字节中。
 *“转换层”（使用transform_angle/Zoom属性）使用较大的缓冲区
 *，不能在块中绘制。因此，这些设置仅影响不透明的小部件。
 */
#define LV_LAYER_SIMPLE_BUF_SIZE          (24 * 1024)
#define LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE (3 * 1024)

/*默认图像缓存大小。图像缓存可以使图像打开。
 *如果仅使用内置图像格式，则没有加缓存的真正优势。 （即，如果未添加新图像解码器）
 *使用复杂的图像解码器（例如PNG或JPG）缓存可以保存图像的连续开放/解码。
 *但是，打开的图像可能会消耗其他RAM。
 *0：禁用缓存*/
#define LV_IMG_CACHE_DEF_SIZE 0

/*每个梯度允许的停止次数。增加这一点以允许更多停止。
 *T他的添加（sizeOf（lv_color_t） + 1）每次额外停止*/
#define LV_GRADIENT_MAX_STOPS 2

/*默认梯度缓冲区大小。
 *当LVGL计算梯度“地图”时，它可以将它们保存到缓存中，以避免再次计算它们。
 *LV_GRAD_CACHE_DEF_SIZE设置该字节中此缓存的大小。
 *如果缓存太小，则仅在图纸需要时分配地图。
 *0 平均没有缓存。*/
#define LV_GRAD_CACHE_DEF_SIZE 0

/*允许抖动梯度（以在有限的颜色深度显示器上实现视觉光滑的颜色梯度）
 *lv_dither_gradient意味着分配对象的渲染表面的一条或两行
 *T如果使用错误扩散 */
#define LV_DITHER_GRADIENT 0
#if LV_DITHER_GRADIENT
    /*增加对错误扩散抖动的支持。
     *错误扩散抖动可获得更好的视觉结果，但意味着绘制时的CPU消耗和内存更多。
     *T他增加的记忆消耗是（24位 *对象的宽度） */
    #define LV_DITHER_ERROR_DIFFUSION 0
#endif

/*最大缓冲尺寸以分配旋转。
 *O如果在显示驱动程序中启用了软件旋转，则使用nly。*/
#define LV_DISP_ROT_MAX_BUF (10*1024)

/*-------------
 *GPU
 *-----------*/

/*使用ARM的2D加速库ARM-2D */
#define LV_USE_GPU_ARM2D 0

/*使用STM32的DMA2D（又名Chrom Art）GPU*/
#define LV_USE_GPU_STM32_DMA2D 0
#if LV_USE_GPU_STM32_DMA2D
    /*必须定义为包括目标处理器的CMSIS标头的路径
    e.g。 “ stm32f7xx.h”或“ stm32f4xx.h”*/
    #define LV_GPU_DMA2D_CMSIS_INCLUDE
#endif

/*启用RA6M3 G2D GPU*/
#define LV_USE_GPU_RA6M3_G2D 0
#if LV_USE_GPU_RA6M3_G2D
    /*包括目标处理器的路径
    e.g。 “ hal_data.h”*/
    #define LV_GPU_RA6M3_G2D_INCLUDE "hal_data.h"
#endif

/*使用SWM341的DMA2D GPU*/
#define LV_USE_GPU_SWM341_DMA2D 0
#if LV_USE_GPU_SWM341_DMA2D
    #define LV_GPU_SWM341_DMA2D_INCLUDE "SWM341.h"
#endif

/*使用NXP的PXP GPU IMX RTXXX平台*/
#define LV_USE_GPU_NXP_PXP 0
#if LV_USE_GPU_NXP_PXP
    /*1：为PXP添加默认的裸机和Freertos中断程序（lv_gpu_nxp_pxp_osa.c）
    *并在lv_init（）中自动调用lv_gpu_nxp_pxp_init（）。请注意，符号sdk_os_free_rtos
    *必须定义为了使用弗雷托斯OSA，否则选择了裸机实现。
    *0：lv_gpu_nxp_pxp_init（）必须在lv_init（）之前手动调用
    */
    #define LV_USE_GPU_NXP_PXP_AUTO_INIT 0
#endif

/*使用NXP的VG-Lite GPU IMX RTXXX平台*/
#define LV_USE_GPU_NXP_VG_LITE 0

/*使用SDL渲染器API*/
#define LV_USE_GPU_SDL 0
#if LV_USE_GPU_SDL
    #define LV_GPU_SDL_INCLUDE_PATH <SDL2/SDL.h>
    /*纹理缓存大小，默认情况下8MB*/
    #define LV_GPU_SDL_LRU_SIZE (1024 * 1024 * 8)
    /*蒙版绘图的自定义混合模式，如果您需要与较旧的SDL2 lib链接，请禁用*/
    #define LV_GPU_SDL_CUSTOM_BLEND_MODE (SDL_VERSION_ATLEAST(2, 0, 6))
#endif

/*-------------
 *记录
 *-----------*/

/*启用日志模块*/
#define LV_USE_LOG 0
#if LV_USE_LOG

    /*应添加重要的日志：
    *lv_log_level_trace许多日志以提供详细信息
    *lv_log_level_info日志重要事件
    *lv_log_level_warn log如果发生了一些事情，但没有引起问题
    *LV_LOG_LEVEL_ERROR仅关键问题，当系统可能失败时
    *LV_LOG_LEVEL_USER仅由用户添加的日志
    *Lv_log_level_none不记录任何东西*/
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /*1：用“ printf”打印日志；
    *0：用户需要用`lv_log_register_print_cb（）注册回调
    #define LV_LOG_PRINTF 0

    /*启用/禁用LV_LOG_TRACE在产生大量日志的模块中*/
    #define LV_LOG_TRACE_MEM        1
    #define LV_LOG_TRACE_TIMER      1
    #define LV_LOG_TRACE_INDEV      1
    #define LV_LOG_TRACE_DISP_REFR  1
    #define LV_LOG_TRACE_EVENT      1
    #define LV_LOG_TRACE_OBJ_CREATE 1
    #define LV_LOG_TRACE_LAYOUT     1
    #define LV_LOG_TRACE_ANIM       1

#endif  /*LV使用日志*/

/*-------------
 *断言
 *-----------*/

/*启用断言是失败还是发现了无效的数据。
 *I启用了f lv_use_log，将在失败上打印错误消息*/
#define LV_USE_ASSERT_NULL          1   /*检查参数是否为null。 （非常快，推荐）*/
#define LV_USE_ASSERT_MALLOC        1   /*检查是记忆成功分配或否。 （非常快，推荐）*/
#define LV_USE_ASSERT_STYLE         0   /*检查样式是否适当初始化。 （非常快，推荐）*/
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /*关键操作后检查`lv_mem`的完整性。 （慢的）*/
#define LV_USE_ASSERT_OBJ           0   /*检查对象的类型和存在（例如未删除）。 （慢的）*/

/*断言发生时添加自定义处理程序，例如重新启动MCU*/
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);   /*默认情况下停止*/

/*-------------
 *其他的
 *-----------*/

/*1：显示CPU使用率和FPS计数*/
#define LV_USE_PERF_MONITOR 0
#if LV_USE_PERF_MONITOR
    #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#endif

/*1：显示二手内存和内存碎片
 * 需要lv_mem_custom = 0*/
#define LV_USE_MEM_MONITOR 0
#if LV_USE_MEM_MONITOR
    #define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT
#endif

/*1：在重新绘制区域上绘制随机彩色矩形*/
#define LV_USE_REFR_DEBUG 0

/*更改内置（V）SNPRINTF函数*/
#define LV_SPRINTF_CUSTOM 0
#if LV_SPRINTF_CUSTOM
    #define LV_SPRINTF_INCLUDE <stdio.h>
    #define lv_snprintf  snprintf
    #define lv_vsnprintf vsnprintf
#else   /*LV Sprintf自定义*/
    #define LV_SPRINTF_USE_FLOAT 0
#endif  /*LV Sprintf自定义*/

#define LV_USE_USER_DATA 1

/*垃圾收集器设置
 *U如果LVGL绑定到更高级别的语言，并且该语言管理内存*/
#define LV_ENABLE_GC 0
#if LV_ENABLE_GC != 0
    #define LV_GC_INCLUDE "gc.h"                           /*包括垃圾收集器相关的东西*/
#endif /*LV启用GC*/

/*====================
 *编译器设置
 *==================*/

/*对于设置为1的大型末日系统*/
#define LV_BIG_ENDIAN_SYSTEM 0

/*将自定义属性定义为`lv_tick_inc`函数*/
#define LV_ATTRIBUTE_TICK_INC

/*将自定义属性定义为`lv_timer_handler`函数*/
#define LV_ATTRIBUTE_TIMER_HANDLER

/*将自定义属性定义为`lv_disp_flush_ready“函数*/
#define LV_ATTRIBUTE_FLUSH_READY

/*缓冲区所需的对齐方式*/
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/*将在需要对齐的记忆的位置添加（默认情况下可能不会将-OS数据对齐到边界）。
 * 例如。 __ attribute __（（（校准（4）））*/
#define LV_ATTRIBUTE_MEM_ALIGN

/*属性标记大型常数阵列，例如字体的位图*/
#define LV_ATTRIBUTE_LARGE_CONST

/*RAM中的大型阵列声明的编译器前缀*/
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/*将绩效关键功能放置在更快的内存中（例如RAM）*/
#define LV_ATTRIBUTE_FAST_MEM

/*GPU加速操作中使用的前缀变量，通常需要将这些变量放在可访问的RAM部分中*/
#define LV_ATTRIBUTE_DMA

/*导出整数恒定为结合。该宏以lv_ <const>的形式与常数一起使用
 *s也应该出现在LVGL结合API上，例如Micropython。*/
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /*默认值刚刚阻止了GCC警告*/

/*将默认的-32K..32K坐标范围扩展到-4m..4m..4m*/
#define LV_USE_LARGE_COORD 0

/*===================
 *使用
 *================== //

/*具有ASCII范围的Montserrat字体和一些使用BPP = 4的符号
 *https：//fonts.google.com/specimen/montserratheth*/
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
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

/*展示特殊功能*/
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /*BPP = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /*希伯来语，阿拉伯语，波斯信件及其所有形式*/
#define LV_FONT_SIMSUN_16_CJK            0  /*1000个最常见的CJK激进分子*/

/*像素完美的单层字体*/
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/*可选地在此处声明自定义字体。
 *您也可以将这些字体作为默认字体使用，它们将在全球可用。
 *E.g。 #define lv_font_custom_declare lv_font_declare（my_font_1）lv_font_declare（my_font_2）*/
#define LV_FONT_CUSTOM_DECLARE

/*始终设置默认字体*/
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*启用具有很多字符的大字体和/或字体。
 *极限取决于字体大小，字体面和bpp。
 *C如果字体需要该字体，将触发错误。*/
#define LV_FONT_FMT_TXT_LARGE 0

/*启用/禁用对压缩字体的支持。*/
#define LV_USE_FONT_COMPRESSED 0

/*启用子像素渲染*/
#define LV_USE_FONT_SUBPX 0
#if LV_USE_FONT_SUBPX
    /*设置显示屏的像素顺序。 RGB通道的物理顺序。与“正常”字体无关紧要。*/
    #define LV_FONT_SUBPX_BGR 0  /*0：RGB; 1：BGR订单*/
#endif

/*当找不到字形DSC时，启用绘图占位符*/
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
 *文本设置
 *================*/

/**
 *选择一个字符串编码的字符。
 *您的IDE或编辑器应具有相同的字符编码
 *-LV_TXT_ENC_UTF8
 *-LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*可以在这些字符上打破（包裹）文本*/
#define LV_TXT_BREAK_CHARS " ,.;:-_"

/*如果一个单词至少这么长，将在“最漂亮”的任何地方打破
 *To禁用，设置为值<= 0*/
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/*在休息之前，最小数量的字符数量要在一条线上戴上。
 *Dlv_txt_line_break_long_len。*/
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/*休息后，最小数量的字符数量要在一条线上放在一条线上。
 *Dlv_txt_line_break_long_len。*/
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/*用于信号文本重新上色的控制字符。*/
#define LV_TXT_COLOR_CMD "#"

/*支持双向文本。允许从左到右和左至左的文本混合。
 *方向将根据Unicode双向算法处理：
 *https：//www.w3.org/international/articles/inline-bidi-markup/uba-basicshy/
#define LV_USE_BIDI 0
#if LV_USE_BIDI
    /*设置默认方向。支持的值：
    *`lv_base_dir_ltr`从左到右
    *`lv_base_dir_rtl`左右
    *`lv_base_dir_auto`检测文本基础方向*/
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/*启用阿拉伯语/波斯加工
 *In这些语言的字符应根据文本中的位置替换为其他表格*/
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*===================
 *小部件用法
 *===============*/

/*小部件的文档：https：//docs.lvgl.io/latest/en/html/widgets/index.html*/

#define LV_USE_ARC        1

#define LV_USE_BAR        1

#define LV_USE_BTN        1

#define LV_USE_BTNMATRIX  1

#define LV_USE_CANVAS     1

#define LV_USE_CHECKBOX   1

#define LV_USE_DROPDOWN   1   /*要求：lv_label*/

#define LV_USE_IMG        1   /*要求：lv_label*/

#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1 /*启用选择标签的文本*/
    #define LV_LABEL_LONG_TXT_HINT 1  /*将一些额外信息存储在标签中以加快绘制很长的文本*/
#endif

#define LV_USE_LINE       1

#define LV_USE_ROLLER     1   /*要求：lv_label*/
#if LV_USE_ROLLER
    #define LV_ROLLER_INF_PAGES 7 /*滚筒无限时的额外“页面”数量*/
#endif

#define LV_USE_SLIDER     1   /*要求：lv_bar*/

#define LV_USE_SWITCH     1

#define LV_USE_TEXTAREA   1   /*要求：lv_label*/
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500    /*多发性硬化症*/
#endif

#define LV_USE_TABLE      1

/*===================
 *额外的组件
 *=================*/

/*-----------
 *小部件
 *----------*/
#define LV_USE_ANIMIMG    1

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
#endif  /*LV使用日历*/

#define LV_USE_CHART      1

#define LV_USE_COLORWHEEL 1

#define LV_USE_IMGBTN     1

#define LV_USE_KEYBOARD   1

#define LV_USE_LED        1

#define LV_USE_LIST       1

#define LV_USE_MENU       1

#define LV_USE_METER      1

#define LV_USE_MSGBOX     1

#define LV_USE_SPAN       1
#if LV_USE_SPAN
    /*线路文本可以包含最大数量的跨度描述符 */
    #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

#define LV_USE_SPINBOX    1

#define LV_USE_SPINNER    1

#define LV_USE_TABVIEW    1

#define LV_USE_TILEVIEW   1

#define LV_USE_WIN        1

/*-----------
 *主题
 *----------*/

/*一个简单，令人印象深刻且非常完整的主题*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT

    /*0：光模式； 1：黑暗模式*/
    #define LV_THEME_DEFAULT_DARK 0

    /*1：启用媒体增长*/
    #define LV_THEME_DEFAULT_GROW 1

    /*[MS]中的默认过渡时间*/
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif /*LV使用主题默认*/

/*一个非常简单的主题，是自定义主题的好起点*/
#define LV_USE_THEME_BASIC 1

/*为单色显示的主题*/
#define LV_USE_THEME_MONO 1

/*-----------
 *布局
 *----------*/

/*类似于CSS的Flexbox的布局。*/
#define LV_USE_FLEX 1

/*与CSS中的网格相似的布局。*/
#define LV_USE_GRID 1

/*---------------------
 *第三方图书馆
 *--------------------*/

/*通用API的文件系统接口 */

/*用于Fopen，Fread等的API*/
#define LV_USE_FS_STDIO 0
#if LV_USE_FS_STDIO
    #define LV_FS_STDIO_LETTER '\0'     /*设置一个上层字母，将驱动器可访问（例如'a'）*/
    #define LV_FS_STDIO_PATH ""         /*设置工作目录。文件/目录路径将附加到其中。*/
    #define LV_FS_STDIO_CACHE_SIZE 0    /*> 0在lv_fs_read（）中缓存此数字字节*/
#endif

/*API打开，阅读等*/
#define LV_USE_FS_POSIX 0
#if LV_USE_FS_POSIX
    #define LV_FS_POSIX_LETTER '\0'     /*设置一个上层字母，将驱动器可访问（例如'a'）*/
    #define LV_FS_POSIX_PATH ""         /*设置工作目录。文件/目录路径将附加到其中。*/
    #define LV_FS_POSIX_CACHE_SIZE 0    /*> 0在lv_fs_read（）中缓存此数字字节*/
#endif

/*用于CreateFile，ReadFile等的API*/
#define LV_USE_FS_WIN32 0
#if LV_USE_FS_WIN32
    #define LV_FS_WIN32_LETTER '\0'     /*设置一个上层字母，将驱动器可访问（例如'a'）*/
    #define LV_FS_WIN32_PATH ""         /*设置工作目录。文件/目录路径将附加到其中。*/
    #define LV_FS_WIN32_CACHE_SIZE 0    /*> 0在lv_fs_read（）中缓存此数字字节*/
#endif

/*FATF的API（需要单独添加）。使用f_open，f_read等*/
#define LV_USE_FS_FATFS 0
#if LV_USE_FS_FATFS
    #define LV_FS_FATFS_LETTER '\0'     /*设置一个上层字母，将驱动器可访问（例如'a'）*/
    #define LV_FS_FATFS_CACHE_SIZE 0    /*> 0在lv_fs_read（）中缓存此数字字节*/
#endif

/*LittleFS的API（需要单独添加库）。使用LFS_FILE_OPEN，LFS_FILE_READ等*/
#define LV_USE_FS_LITTLEFS 0
#if LV_USE_FS_LITTLEFS
    #define LV_FS_LITTLEFS_LETTER '\0'     /*设置一个上层字母，将驱动器可访问（例如'a'）*/
    #define LV_FS_LITTLEFS_CACHE_SIZE 0    /*> 0在lv_fs_read（）中缓存此数字字节*/
#endif

/*PNG解码器库*/
#define LV_USE_PNG 0

/*BMP解码器库*/
#define LV_USE_BMP 0

/* JPG +拆分JPG解码器库。
 * Split JPG是针对嵌入式系统优化的自定义格式。 */
#define LV_USE_SJPG 0

/*GIF解码器库*/
#define LV_USE_GIF 0

/*QR代码库*/
#define LV_USE_QRCODE 0

/*Freetype库*/
#define LV_USE_FREETYPE 0
#if LV_USE_FREETYPE
    /*Freetype用来缓存字符[字节]（-1：无缓存）使用的内存*/
    #define LV_FREETYPE_CACHE_SIZE (16 * 1024)
    #if LV_FREETYPE_CACHE_SIZE >= 0
        /* 1：位图缓存使用SBIT缓存，0：位图缓存使用图像缓存。 */
        /* SBIT缓存：对于小位图（字体尺寸<256），它是更有效的内存效率 */
        /* 如果字体大小> = 256，则必须配置为图像缓存 */
        #define LV_FREETYPE_SBIT_CACHE 0
        /* 此缓存实例管理的最大打开ft_face/ft_size对象。 */
        /* （0：使用系统默认值） */
        #define LV_FREETYPE_CACHE_FT_FACES 0
        #define LV_FREETYPE_CACHE_FT_SIZES 0
    #endif
#endif

/*Tiny TTF库*/
#define LV_USE_TINY_TTF 0
#if LV_USE_TINY_TTF
    /*从文件加载TTF数据*/
    #define LV_TINY_TTF_FILE_SUPPORT 0
#endif

/*Rlottie库*/
#define LV_USE_RLOTTIE 0

/*用于图像解码和播放视频的FFMPEG库
 *S在所有主要图像格式上，因此请勿使用它启用其他图像解码器*/
#define LV_USE_FFMPEG 0
#if LV_USE_FFMPEG
    /*转储输入信息到stderr*/
    #define LV_FFMPEG_DUMP_FORMAT 0
#endif

/*-----------
 *其他的
 *----------*/

/*1：允许API拍摄对象的快照*/
#define LV_USE_SNAPSHOT 0

/*1：启用猴子测试*/
#define LV_USE_MONKEY 0

/*1：启用电网导航*/
#define LV_USE_GRIDNAV 0

/*1：启用LV_OBJ片段*/
#define LV_USE_FRAGMENT 0

/*1：支持使用图像作为标签或跨度小部件字体的支持 */
#define LV_USE_IMGFONT 0

/*1：启用基于订户的消息传递系统 */
#define LV_USE_MSG 0

/*1：启用拼音输入方法*/
/*要求：lv_keyboard*/
#define LV_USE_IME_PINYIN 0
#if LV_USE_IME_PINYIN
    /*1：使用默认的词库*/
    /*如果您不使用默认的词库，请确保在设置thesauruss之后使用`lv_ime_pinyin`*/
    #define LV_IME_PINYIN_USE_DEFAULT_DICT 1
    /*设置最大数量可以显示的候选面板*/
    /*需要根据屏幕的大小进行调整*/
    #define LV_IME_PINYIN_CAND_TEXT_NUM 6

    /*使用9个密钥输入（K9）*/
    #define LV_IME_PINYIN_USE_K9_MODE      1
    #if LV_IME_PINYIN_USE_K9_MODE == 1
        #define LV_IME_PINYIN_K9_CAND_TEXT_NUM 3
    #endif // LV IME Pinyin使用K9模式
#endif

/*===================
*示例
*=================*/

/*使示例可以与库一起构建*/
#define LV_BUILD_EXAMPLES 1

/*===================
 *演示使用量
 ===================*/

/*显示一些小部件。可能需要增加`lv_mem_size` */
#define LV_USE_DEMO_WIDGETS 0
#if LV_USE_DEMO_WIDGETS
#define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif

/*演示用途编码器和键盘*/
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

/*基准您的系统*/
#define LV_USE_DEMO_BENCHMARK 0
#if LV_USE_DEMO_BENCHMARK
/*使用具有16位颜色深度的RGB565A8图像而不是ARGB8565*/
#define LV_DEMO_BENCHMARK_RGB565A8 0
#endif

/*LVGL的压力测试*/
#define LV_USE_DEMO_STRESS 0

/*音乐播放器演示*/
#define LV_USE_DEMO_MUSIC 0
#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_SQUARE    0
    #define LV_DEMO_MUSIC_LANDSCAPE 0
    #define LV_DEMO_MUSIC_ROUND     0
    #define LV_DEMO_MUSIC_LARGE     0
    #define LV_DEMO_MUSIC_AUTO_PLAY 0
#endif

/* - lv_conf_h-的端*/

#endif /*LV conf h*/

#endif /*“内容启用”的结尾*/
