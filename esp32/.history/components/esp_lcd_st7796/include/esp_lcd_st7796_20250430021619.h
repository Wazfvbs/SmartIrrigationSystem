/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief ESP LCD: ST7796
 */

#pragma once

#include "hal/lcd_types.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_idf_version.h"
#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_mipi_dsi.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD panel initialization commands.
 *
 */
typedef struct {
    int cmd;                /*<! The specific LCD command */
    const void *data;       /*<! Buffer that holds the command specific data */
    size_t data_bytes;      /*<! Size of `data` in memory, in bytes */
    unsigned int delay_ms;  /*<! Delay in milliseconds after this command */
} st7796_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *
 * @note  This structure needs to be passed to the `vendor_config` field in `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct {
    const st7796_lcd_init_cmd_t *init_cmds;     /*!< Pointer to initialization commands array. Set to NULL if using default commands.
                                                 *   The array should be declared as `static const` and positioned outside the function.
                                                 *   Please refer to `vendor_specific_init_default` in source file.
                                                 */
    uint16_t init_cmds_size;                    /*<! Number of commands in above array */
#if SOC_MIPI_DSI_SUPPORTED
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus;               /*!< MIPI-DSI bus configuration */
        const esp_lcd_dpi_panel_config_t *dpi_config;   /*!< MIPI-DPI panel configuration */
    } mipi_config;
#endif
    struct {
        unsigned int use_mipi_interface: 1;         /*<! Set to 1 if using MIPI interface, default is SPI/I80 interface */
    } flags;
} st7796_vendor_config_t;

/**
 * @brief Create LCD panel for model ST7796
 *
 * @note  Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for initialization sequence code.
 *
 * @param[in] io LCD panel IO handle
 * @param[in] panel_dev_config general panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_st7796(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);
esp_err_t esp_lcd_new_panel_st7796(const esp_lcd_panel_io_handle_t io,
                                   const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Default Configuration Macros for I80 Interface /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief LCD panel bus configuration structure
 *
 * @param[in] max_trans_bytes Maximum bytes of data to be transferred in one transaction. Normally set to the size of one frame bytes.
 * @param[in] data_width Data bus width
 * @param[in] dc I80 data/command pin number
 * @param[in] wr I80 write clock pin number
 * @param[in] d[0:15] I80 data pin number 0 ~ 15. Set to -1 if not used.
 *
 */
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 4, 1)
#define ST7796_PANEL_BUS_I80_CONFIG(max_trans_bytes, data_width, dc, wr, d0, d1, d2, d3, d4,    \
                                    d5, d6, d7, d8 , d9, d10, d11, d12, d13, d14, d15)          \
    {                                                                                           \
        .clk_src = LCD_CLK_SRC_PLL160M,                                                         \
        .dc_gpio_num = dc,                                                                      \
        .wr_gpio_num = wr,                                                                      \
        .data_gpio_nums = {                                                                     \
            d0, d1, d2, d3, d4, d5, d6, d7, d8 , d9, d10, d11, d12, d13, d14, d15               \
        },                                                                                      \
        .bus_width = data_width,                                                                \
        .max_transfer_bytes = max_trans_bytes,                                                  \
    }
#else
#define ST7796_PANEL_BUS_I80_CONFIG(max_trans_bytes, data_width, dc, wr, d0, d1, d2, d3, d4,    \
                                    d5, d6, d7, d8 , d9, d10, d11, d12, d13, d14, d15)          \
    {                                                                                           \
        .clk_src = LCD_CLK_SRC_PLL160M,                                                         \
        .dc_gpio_num = dc,                                                                      \
        .wr_gpio_num = wr,                                                                      \
        .data_gpio_nums = {                                                                     \
            d0, d1, d2, d3, d4, d5, d6, d7, d8 , d9, d10, d11, d12, d13, d14, d15               \
        },                                                                                      \
        .bus_width = data_width,                                                                \
        .max_transfer_bytes = max_trans_bytes,                                                  \
        .psram_trans_align = 64,                                                                \
        .sram_trans_align = 4,                                                                  \
    }
#endif

/**
 * @brief LCD panel IO configuration structure
 *
 * @param[in] cs I80 chip select pin number
 * @param[in] cb Callback function when data transfer is done
 * @param[in] cb_ctx Callback function context
 *
 */
#define ST7796_PANEL_IO_I80_CONFIG(cs, cb, cb_ctx)  \
    {                                               \
        .cs_gpio_num = cs,                          \
        .pclk_hz = 10 * 1000 * 1000,                \
        .trans_queue_depth = 10,                    \
        .dc_levels = {                              \
            .dc_idle_level = 0,                     \
            .dc_cmd_level = 0,                      \
            .dc_dummy_level = 0,                    \
            .dc_data_level = 1,                     \
        },                                          \
        .flags = {                                  \
            .swap_color_bytes = 1,                  \
        },                                          \
        .on_color_trans_done = cb,                  \
        .user_ctx = cb_ctx,                         \
        .lcd_cmd_bits = 8,                          \
        .lcd_param_bits = 8,                        \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Default Configuration Macros for MIPI-DSI Interface //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief MIPI-DSI bus configuration structure
 *
 * @param[in] lane_num Number of data lanes
 * @param[in] lane_mbps Lane bit rate in Mbps
 *
 */
#define ST7796_PANEL_BUS_DSI_1CH_CONFIG()                \
    {                                                    \
        .bus_id = 0,                                     \
        .num_data_lanes = 1,                             \
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,     \
        .lane_bit_rate_mbps = 480,                       \
    }

/**
 * @brief MIPI-DBI panel IO configuration structure
 *
 */
#define ST7796_PANEL_IO_DBI_CONFIG()  \
    {                                 \
        .virtual_channel = 0,         \
        .lcd_cmd_bits = 8,            \
        .lcd_param_bits = 8,          \
    }

/**
 * @brief MIPI DPI configuration structure
 *
 * @note  refresh_rate = (dpi_clock_freq_mhz * 1000000) / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
 *                                                      / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
 *
 * @param[in] px_format Pixel format of the panel
 *
 */
#define ST7796_320_480_PANEL_60HZ_DPI_CONFIG(px_format)  \
    {                                                    \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,     \
        .dpi_clock_freq_mhz = 10,                        \
        .virtual_channel = 0,                            \
        .pixel_format = px_format,                       \
        .num_fbs = 1,                                    \
        .video_timing = {                                \
            .h_size = 320,                               \
            .v_size = 480,                               \
            .hsync_back_porch = 10,                      \
            .hsync_pulse_width = 10,                     \
            .hsync_front_porch = 20,                     \
            .vsync_back_porch = 10,                      \
            .vsync_pulse_width = 10,                     \
            .vsync_front_porch = 10,                     \
        },                                               \
        .flags.use_dma2d = true,                         \
    }

#ifdef __cplusplus
}
#endif
