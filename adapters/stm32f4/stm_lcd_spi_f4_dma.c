/** Optional STM32F4 normal-mode TX DMA; ISR mailbox, main-loop ownership. */
#include "stm_lcd_spi_f4.h"
#include <limits.h>

static void release_bus(stm_lcd_spi_f4_t *t)
{
    HAL_GPIO_WritePin(t->config.cs_port, t->config.cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(t->config.dc_port, t->config.dc_pin, GPIO_PIN_SET);
}

static void mask_irqs(stm_lcd_spi_f4_t *t)
{
    HAL_NVIC_DisableIRQ(t->config.dma_irq);
    HAL_NVIC_DisableIRQ(t->config.spi_irq);
    __DSB();
    __ISB();
}

static void clear_pending_irqs(stm_lcd_spi_f4_t *t)
{
    HAL_NVIC_ClearPendingIRQ(t->config.dma_irq);
    HAL_NVIC_ClearPendingIRQ(t->config.spi_irq);
}

static void unmask_irqs(stm_lcd_spi_f4_t *t)
{
    HAL_NVIC_EnableIRQ(t->config.dma_irq);
    HAL_NVIC_EnableIRQ(t->config.spi_irq);
}

/* SysTick remains enabled during HAL's bounded abort. Even an error callback
 * may have already cleared TXDMAEN: explicitly stop a still-enabled stream.
 * If hardware refuses to stop, reset rather than release a borrowed buffer. */
static int stop_transfer(stm_lcd_spi_f4_t *t)
{
    HAL_StatusTypeDef err = HAL_SPI_Abort(t->config.spi);
    CLEAR_BIT(t->config.spi->Instance->CR2, SPI_CR2_TXDMAEN);
    __HAL_SPI_DISABLE(t->config.spi);
    if (t->config.spi->hdmatx->Instance->CR & DMA_SxCR_EN)
    {
        if (HAL_DMA_Abort(t->config.spi->hdmatx) != HAL_OK)
            err = HAL_ERROR;
    }
    if (err != HAL_OK)
        ++t->stats.abort_errors;
    if (t->config.spi->hdmatx->Instance->CR & DMA_SxCR_EN)
    {
        NVIC_SystemReset();
        return 0; /* Reset never returns on the MCU; retain ownership in mocks. */
    }
    return 1;
}

stm_err_t stm_lcd_spi_f4_tx_color_async(void *io, uint8_t command, const void *pixels, size_t bytes)
{
    stm_lcd_spi_f4_t *t = io;
    if (!t || !t->initialized || !t->config.enable_dma)
        return STM_ERR_INVALID_ARG;
    if (t->active)
        return STM_ERR_INVALID_STATE;
    if (!pixels || !bytes || !t->config.spi->hdmatx)
        return STM_ERR_INVALID_ARG;
    if (bytes > UINT16_MAX)
        return STM_ERR_OUT_OF_RANGE;
    /* Reject inaccessible memory before selecting the bus, including CCM. */
    uintptr_t address = (uintptr_t)pixels;
    if (address < t->config.dma_memory_begin || address >= t->config.dma_memory_end ||
        bytes > t->config.dma_memory_end - address)
        return STM_ERR_INVALID_ARG;
    HAL_GPIO_WritePin(t->config.cs_port, t->config.cs_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(t->config.dc_port, t->config.dc_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(t->config.spi, &command, 1, t->config.timeout_ms);
    if (status != HAL_OK)
    {
        release_bus(t);
        return status == HAL_TIMEOUT ? STM_ERR_TIMEOUT : STM_ERR_IO;
    }
    HAL_GPIO_WritePin(t->config.dc_port, t->config.dc_pin, GPIO_PIN_SET);
    mask_irqs(t);
    clear_pending_irqs(t);
    t->started = HAL_GetTick();
    t->irq_result = 0;
    t->active = 1; /* Publish before enabling DMA: an IRQ may be immediate. */
    __DMB();
    status = HAL_SPI_Transmit_DMA(t->config.spi, (uint8_t *)pixels, (uint16_t)bytes);
    if (status != HAL_OK)
    {
        if (!stop_transfer(t))
            return STM_ERR_IO;
        release_bus(t);
        t->active = 0;
        t->irq_result = 0;
        ++t->stats.errors;
        clear_pending_irqs(t);
        unmask_irqs(t);
        return status == HAL_TIMEOUT ? STM_ERR_TIMEOUT : STM_ERR_IO;
    }
    ++t->stats.submitted;
    unmask_irqs(t);
    return STM_OK;
}

/* HAL's normal-mode TX callback is after TXE/BSY and State=READY. These
 * callbacks only publish a mailbox; no LVGL, logging, I2C or CS changes. */
void stm_lcd_spi_f4_irq_complete(stm_lcd_spi_f4_t *t, SPI_HandleTypeDef *spi)
{
    if (!t || !t->initialized || spi != t->config.spi)
        return;
    if (!t->active)
    {
        ++t->stats.unexpected_irqs;
        return;
    }
    __DMB();
    if (t->irq_result != 2)
        t->irq_result = 1;
}

void stm_lcd_spi_f4_irq_error(stm_lcd_spi_f4_t *t, SPI_HandleTypeDef *spi)
{
    if (!t || !t->initialized || spi != t->config.spi)
        return;
    if (!t->active)
    {
        ++t->stats.unexpected_irqs;
        return;
    }
    t->stats.last_hal_error = spi->ErrorCode;
    __DMB();
    t->irq_result = 2;
}

static int transfer_finished(stm_lcd_spi_f4_t *t, uint8_t result)
{
    return result == 1 && HAL_SPI_GetState(t->config.spi) == HAL_SPI_STATE_READY &&
           !(t->config.spi->Instance->SR & SPI_FLAG_BSY) &&
           !(t->config.spi->Instance->CR2 & SPI_CR2_TXDMAEN) &&
           !(t->config.spi->hdmatx->Instance->CR & DMA_SxCR_EN);
}

void stm_lcd_spi_f4_poll(stm_lcd_spi_f4_t *t)
{
    if (!t || !t->initialized || !t->active)
        return;
    uint32_t elapsed = HAL_GetTick() - t->started;
    uint8_t result = t->irq_result;
    __DMB();
    if (!transfer_finished(t, result) && result != 2 && elapsed < t->config.timeout_ms)
        return;
    mask_irqs(t);
    /* Re-read after masking: completion/error may have arrived since the peek. */
    result = t->irq_result;
    __DMB();
    elapsed = HAL_GetTick() - t->started;
    int bus_done = transfer_finished(t, result);
    if (!bus_done && result != 2 && elapsed < t->config.timeout_ms)
    {
        unmask_irqs(t);
        return;
    }
    stm_err_t err = STM_OK;
    if (!bus_done)
    {
        err = result == 2 ? STM_ERR_IO : STM_ERR_TIMEOUT;
        ++t->stats.errors;
        if (err == STM_ERR_TIMEOUT)
            ++t->stats.timeouts;
        if (!stop_transfer(t))
            return;
    }
    release_bus(t); /* Wire and DMA are quiescent before ownership is returned. */
    t->active = 0;
    t->irq_result = 0;
    t->stats.total_ms += elapsed;
    if (elapsed > t->stats.max_ms)
        t->stats.max_ms = elapsed;
    if (err == STM_OK)
        ++t->stats.completed;
    clear_pending_irqs(t);
    unmask_irqs(t);
    (void)stm_lcd_io_complete(&t->io, err); /* Main loop, DMA and wire stopped. */
}
