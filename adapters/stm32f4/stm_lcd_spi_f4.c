#include "stm_lcd_spi_f4.h"
#include <limits.h>
#include <string.h>
#ifndef STM_LCD_SPI_F4_DMA
#define STM_LCD_SPI_F4_DMA 1
#endif

#if STM_LCD_SPI_F4_DMA
static void process(void *context)
{
    stm_lcd_spi_f4_poll(context);
}
#endif
static int busy(void *context)
{
    return stm_lcd_spi_f4_busy(context);
}

static const stm_lcd_io_ops_t sync_ops = {
    .tx_param = stm_lcd_spi_f4_tx_param,
    .tx_color = stm_lcd_spi_f4_tx_color,
    .busy = busy,
};
#if STM_LCD_SPI_F4_DMA
static const stm_lcd_io_ops_t dma_ops = {
    .tx_param = stm_lcd_spi_f4_tx_param,
    .tx_color = stm_lcd_spi_f4_tx_color,
    .tx_color_async = stm_lcd_spi_f4_tx_color_async,
    .process = process,
    .busy = busy,
};
#else
#define dma_ops sync_ops
#endif
stm_err_t stm_lcd_spi_f4_deinit(stm_lcd_spi_f4_t *t)
{
    if (!t || !t->initialized)
        return STM_ERR_INVALID_ARG;
    stm_err_t err = stm_lcd_io_deinit(&t->io);
    if (err == STM_OK)
        memset(t, 0, sizeof(*t));
    return err;
}

static stm_err_t from_hal(HAL_StatusTypeDef result)
{
    return result == HAL_OK ? STM_OK : result == HAL_TIMEOUT ? STM_ERR_TIMEOUT : STM_ERR_IO;
}

stm_err_t stm_lcd_spi_f4_init(stm_lcd_spi_f4_t *t, const stm_lcd_spi_f4_config_t *config)
{
    if (!t || !config)
        return STM_ERR_INVALID_ARG;
    if (t->active || t->initialized)
        return STM_ERR_INVALID_STATE;
    if (!config->spi || !config->cs_port || !config->dc_port || !config->timeout_ms ||
        config->timeout_ms > INT32_MAX || config->enable_dma > 1)
        return STM_ERR_INVALID_CONFIG;
    if (config->enable_dma)
    {
#if STM_LCD_SPI_F4_DMA
        if (!config->spi->Instance || !config->spi->hdmatx || !config->spi->hdmatx->Instance ||
            !config->dma_memory_begin || config->dma_memory_end <= config->dma_memory_begin ||
            config->dma_irq < 0 || config->spi_irq < 0 || config->dma_irq == config->spi_irq)
            return STM_ERR_INVALID_CONFIG;
#else
        return STM_ERR_INVALID_CONFIG;
#endif
    }
    memset(t, 0, sizeof(*t));
    t->config = *config;
    stm_err_t result = stm_lcd_io_init(&t->io, config->enable_dma ? &dma_ops : &sync_ops, t);
    if (result != STM_OK)
        return result;
    t->initialized = 1;
    return STM_OK;
}

int stm_lcd_spi_f4_busy(const stm_lcd_spi_f4_t *t)
{
    return t && t->active;
}

static stm_err_t write_bytes(stm_lcd_spi_f4_t *t, const uint8_t *data, size_t bytes)
{
    while (bytes)
    {
        uint16_t n = bytes > 4096u ? 4096u : (uint16_t)bytes;
        stm_err_t result =
            from_hal(HAL_SPI_Transmit(t->config.spi, (uint8_t *)data, n, t->config.timeout_ms));
        if (result != STM_OK)
            return result;
        data += n;
        bytes -= n;
    }
    return STM_OK;
}

stm_err_t stm_lcd_spi_f4_tx_param(void *io, uint8_t cmd, const uint8_t *data, size_t bytes)
{
    stm_lcd_spi_f4_t *t = io;
    if (!t || !t->initialized || (bytes && !data))
        return STM_ERR_INVALID_ARG;
    if (t->active)
        return STM_ERR_INVALID_STATE;
    HAL_GPIO_WritePin(t->config.cs_port, t->config.cs_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(t->config.dc_port, t->config.dc_pin, GPIO_PIN_RESET);
    stm_err_t result = write_bytes(t, &cmd, 1);
    if (result == STM_OK && bytes)
    {
        HAL_GPIO_WritePin(t->config.dc_port, t->config.dc_pin, GPIO_PIN_SET);
        result = write_bytes(t, data, bytes);
    }
    HAL_GPIO_WritePin(t->config.cs_port, t->config.cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(t->config.dc_port, t->config.dc_pin, GPIO_PIN_SET);
    return result;
}

stm_err_t stm_lcd_spi_f4_tx_color(void *io, uint8_t cmd, const void *data, size_t bytes)
{
    return stm_lcd_spi_f4_tx_param(io, cmd, data, bytes);
}
