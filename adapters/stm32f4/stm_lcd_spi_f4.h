/**
 * @file    stm_lcd_spi_f4.h
 * @brief   提供可选 STM32F4 HAL SPI 与 DMA 适配接口。
 */
#ifndef STM_LCD_SPI_F4_H
#define STM_LCD_SPI_F4_H
#include "stm32f4xx_hal.h"
#include "stm_lcd_impl.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    SPI_HandleTypeDef *spi;                     // 借用的已初始化 SPI，适配独占使用。
    GPIO_TypeDef *cs_port, *dc_port;            // CS 与 DC 的 GPIO 端口。
    uint16_t cs_pin, dc_pin;                    // GPIO 掩码，CS 低有效。
    uint32_t timeout_ms;                        // 非零且不超过 INT32_MAX，覆盖 DMA 生命周期。
    uint8_t enable_dma;                         // 显式启用 DMA，同步模式无需启用。
    IRQn_Type dma_irq, spi_irq;                 // 专用 IRQ，提交前必须启用。
    uintptr_t dma_memory_begin, dma_memory_end; // 可 DMA 访问的 SRAM 半开区间。
} stm_lcd_spi_f4_config_t;

typedef struct
{
    struct stm_lcd_io io;                // 调用方持有的嵌入式 IO base。
    stm_lcd_spi_f4_config_t config;      // 配置副本，初始化后不修改。
    uint32_t started;                    // 内部起始时间。
    volatile uint8_t active, irq_result; // ISR 状态邮箱，不调用 LVGL。
    uint8_t initialized;                 // 首次初始化前清零。

    volatile struct
    {
        uint32_t submitted, completed, errors, timeouts;
        uint32_t abort_errors, last_hal_error, unexpected_irqs;
        uint32_t total_ms, max_ms;
    } stats; // 诊断计数，不作为所有权判据。
} stm_lcd_spi_f4_t;

/**
 * @brief 清除适配对象，不销毁 HAL 外设。
 *
 * @param transport 待清除适配对象
 * @return STM_OK；有借用者或在途操作时返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_spi_f4_deinit(stm_lcd_spi_f4_t *transport);

/**
 * @brief 初始化调用方持有的清零 STM32F4 SPI 适配，不初始化硬件。
 *
 * @param transport 持续有效的已清零适配对象
 * @param config 已初始化 HAL 句柄及有效配置；复制后不修改
 * @return STM_OK；参数、配置或重复初始化错误时失败。
 */
stm_err_t stm_lcd_spi_f4_init(stm_lcd_spi_f4_t *transport, const stm_lcd_spi_f4_config_t *config);

/**
 * @brief 适配内部同步命令入口，使用者应调用通用 IO 接口。
 *
 * @param io SPI 适配实现上下文，不是通用 IO base
 * @param cmd 控制器命令字节
 * @param data 参数或像素字节缓冲
 * @param bytes 传输字节数
 * @return STM_OK 或参数、配置、状态及 HAL 传输错误。
 */
stm_err_t stm_lcd_spi_f4_tx_param(void *io, uint8_t cmd, const uint8_t *data, size_t bytes);

/**
 * @brief 适配内部同步像素入口，使用者应调用通用 IO 接口。
 *
 * @param io SPI 适配实现上下文，不是通用 IO base
 * @param cmd 控制器命令字节
 * @param data 参数或像素字节缓冲
 * @param bytes 传输字节数
 * @return STM_OK 或参数、配置、状态及 HAL 传输错误。
 */
stm_err_t stm_lcd_spi_f4_tx_color(void *io, uint8_t cmd, const void *data, size_t bytes);

/**
 * @brief 适配内部 DMA 提交入口，使用者应调用通用 IO 接口。
 *
 * @param io SPI 适配实现上下文，不是通用 IO base
 * @param cmd 控制器命令字节
 * @param data 参数或像素字节缓冲
 * @param bytes 传输字节数
 * @return STM_OK 或传输错误；失败但硬件仍忙时不能归还异步缓冲。
 */
stm_err_t stm_lcd_spi_f4_tx_color_async(void *io, uint8_t cmd, const void *data, size_t bytes);

/**
 * @brief 在主循环确认停止并通知完成；通常由通用 IO process 调用。
 *
 * @param transport 已初始化的 SPI 适配对象
 */
void stm_lcd_spi_f4_poll(stm_lcd_spi_f4_t *transport);

/**
 * @brief 查询 DMA 是否仍借用像素。
 *
 * @param transport 待查询适配对象
 * @return 非零表示仍占用；空对象返回零。
 */
int stm_lcd_spi_f4_busy(const stm_lcd_spi_f4_t *transport);

/**
 * @brief 显式转发 HAL SPI 完成 IRQ，仅记录邮箱状态。
 *
 * @param transport 持续有效的 SPI 适配对象
 * @param spi HAL 回调中的句柄；非目标外设忽略
 */
void stm_lcd_spi_f4_irq_complete(stm_lcd_spi_f4_t *transport, SPI_HandleTypeDef *spi);

/**
 * @brief 显式转发 HAL SPI 错误 IRQ，仅记录邮箱状态。
 *
 * @param transport 持续有效的 SPI 适配对象
 * @param spi HAL 回调中的句柄；非目标外设忽略
 */
void stm_lcd_spi_f4_irq_error(stm_lcd_spi_f4_t *transport, SPI_HandleTypeDef *spi);
#ifdef __cplusplus
}
#endif
#endif
