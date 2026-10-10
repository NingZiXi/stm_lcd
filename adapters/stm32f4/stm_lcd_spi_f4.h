/** @file stm_lcd_spi_f4.h @brief Optional exclusive-bus STM32F4 HAL SPI transport.
 * No panel commands, LVGL, board headers, GPIO assignments or global HAL callbacks.
 * Caller initializes HAL/IRQs first and owns a zero-initialized context for its lifetime.
 * Main-loop calls only, except irq_complete/irq_error; one owner per SPI peripheral.
 * DMA requires normal byte TX, enabled dedicated SPI/DMA IRQs, and an accessible
 * contiguous SRAM range [dma_memory_begin,dma_memory_end); CCM is not accessible.
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
    SPI_HandleTypeDef *spi;          /**< Borrowed, exclusively owned SPI. */
    GPIO_TypeDef *cs_port, *dc_port; /**< Command/data and chip-select wiring. */
    uint16_t cs_pin, dc_pin;         /**< GPIO masks, active-low CS. */
    uint32_t timeout_ms;             /**< Nonzero, less than INT32_MAX; includes DMA lifetime. */
    uint8_t enable_dma;              /**< Explicit opt-in; synchronous use needs no DMA. */
    IRQn_Type dma_irq, spi_irq;      /**< Dedicated IRQs, enabled before submission. */
    uintptr_t dma_memory_begin, dma_memory_end; /**< Required accessible SRAM range. */
} stm_lcd_spi_f4_config_t;

typedef struct
{
    struct stm_lcd_io io;           /**< Generic object embedded in caller-owned adapter. */
    stm_lcd_spi_f4_config_t config; /**< Copied configuration; do not modify while initialized. */
    uint32_t started;               /**< Internal timing state. */
    volatile uint8_t active, irq_result; /**< ISR mailbox only, no LVGL in IRQ. */
    uint8_t initialized;                 /**< Zero-initialize before first init. */

    volatile struct
    {
        uint32_t submitted, completed, errors, timeouts;
        uint32_t abort_errors, last_hal_error, unexpected_irqs;
        uint32_t total_ms, max_ms;
    } stats; /**< Optional inspection counters, never ownership decisions. */
} stm_lcd_spi_f4_t;

stm_err_t stm_lcd_spi_f4_deinit(stm_lcd_spi_f4_t *transport);
stm_err_t stm_lcd_spi_f4_init(stm_lcd_spi_f4_t *transport, const stm_lcd_spi_f4_config_t *config);
/** Adapter implementation methods; consumers use stm_lcd_io_* on &transport->io. */
stm_err_t stm_lcd_spi_f4_tx_param(void *io, uint8_t cmd, const uint8_t *data, size_t bytes);
stm_err_t stm_lcd_spi_f4_tx_color(void *io, uint8_t cmd, const void *data, size_t bytes);
/** Success borrows pixels until complete; failed submission stops DMA before returning. */
stm_err_t stm_lcd_spi_f4_tx_color_async(void *io, uint8_t cmd, const void *data, size_t bytes);
void stm_lcd_spi_f4_poll(stm_lcd_spi_f4_t *transport);
int stm_lcd_spi_f4_busy(const stm_lcd_spi_f4_t *transport);
/** Explicitly forward the application's HAL SPI callbacks; other SPI is ignored. */
void stm_lcd_spi_f4_irq_complete(stm_lcd_spi_f4_t *transport, SPI_HandleTypeDef *spi);
void stm_lcd_spi_f4_irq_error(stm_lcd_spi_f4_t *transport, SPI_HandleTypeDef *spi);
#ifdef __cplusplus
}
#endif
#endif
