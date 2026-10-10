/**
 * @file    stm_lcd_io.c
 * @brief   实现 IO 生命周期及安全完成分发。
 */
#include "stm_lcd_impl.h"
#include <string.h>

stm_err_t stm_lcd_io_init(stm_lcd_io_handle_t io, const stm_lcd_io_ops_t *ops, void *context)
{
    if (!io || !ops)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (io->ops)
    {
        return STM_ERR_INVALID_STATE;
    }
    io->ops = ops;
    io->context = context;
    return STM_OK;
}

int stm_lcd_io_busy(stm_lcd_io_handle_t io)
{
    return io && (io->pending || (io->ops && io->ops->busy && io->ops->busy(io->context)));
}

int stm_lcd_io_supports_async(stm_lcd_io_handle_t io)
{
    return io && io->ops && io->ops->tx_color_async && io->ops->process;
}

stm_err_t stm_lcd_io_complete(stm_lcd_io_handle_t io, stm_err_t result)
{
    if (!io || !io->ops)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (!io->pending || io->dispatching || (io->ops->busy && io->ops->busy(io->context)))
    {
        return STM_ERR_INVALID_STATE;
    }
    if (io->scanout && io->frame_error == STM_OK && result != STM_OK)
    {
        io->frame_error = result;
    }
    if (io->scanout && io->frame_error != STM_OK)
    {
        stm_err_t stop = io->ops->stop_scanout(io->context);
        if (stop != STM_OK)
        {
            return stop;
        }
        io->scanout = 0;
    }
    if (io->frame_error != STM_OK)
    {
        result = io->frame_error;
    }
    io->frame_error = STM_OK;
    io->pending = 0;
    io->dispatching = 1;
    if (io->complete)
    {
        io->complete(io->owner, result);
    }
    io->dispatching = 0;
    return STM_OK;
}

static stm_err_t check(stm_lcd_io_handle_t io)
{
    if (!io || !io->ops)
    {
        return STM_ERR_INVALID_ARG;
    }
    return stm_lcd_io_busy(io) || io->dispatching ? STM_ERR_INVALID_STATE : STM_OK;
}

stm_err_t stm_lcd_io_tx_param(stm_lcd_io_handle_t io, uint8_t cmd, const uint8_t *data, size_t n)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (n && !data)
    {
        return STM_ERR_INVALID_ARG;
    }
    return io->ops->tx_param ? io->ops->tx_param(io->context, cmd, data, n) : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_io_tx_color(stm_lcd_io_handle_t io, uint8_t cmd, const void *data, size_t n)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!data || !n)
    {
        return STM_ERR_INVALID_ARG;
    }
    return io->ops->tx_color ? io->ops->tx_color(io->context, cmd, data, n) : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_io_tx_color_async(stm_lcd_io_handle_t io, uint8_t cmd, const void *data, size_t n)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!data || !n)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (!stm_lcd_io_supports_async(io))
    {
        return STM_ERR_NOT_SUPPORTED;
    }
    io->pending = 1;
    err = io->ops->tx_color_async(io->context, cmd, data, n);
    // 硬件仍忙时保留像素所有权。
    if (err != STM_OK && io->pending && (!io->ops->busy || !io->ops->busy(io->context)))
    {
        io->pending = 0;
    }
    return err;
}

stm_err_t stm_lcd_io_write(stm_lcd_io_handle_t io, const uint8_t *data, size_t n)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!data || !n)
    {
        return STM_ERR_INVALID_ARG;
    }
    return io->ops->write ? io->ops->write(io->context, data, n) : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_io_read(stm_lcd_io_handle_t io, uint8_t *data, size_t n)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!data || !n)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (!io->ops->read)
    {
        return STM_ERR_NOT_SUPPORTED;
    }
    err = io->ops->read(io->context, data, n);
    if (err == STM_OK && io->read_observer)
    {
        io->dispatching = 1;
        io->read_observer(io->read_context, data, n);
        io->dispatching = 0;
    }
    return err;
}

stm_err_t stm_lcd_io_probe(stm_lcd_io_handle_t io)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    return io->ops->probe ? io->ops->probe(io->context) : STM_ERR_NOT_SUPPORTED;
}

void stm_lcd_io_process(stm_lcd_io_handle_t io)
{
    if (io && io->ops && !io->dispatching && io->ops->process)
    {
        io->ops->process(io->context);
    }
}

stm_err_t
stm_lcd_io_set_read_observer(stm_lcd_io_handle_t io, stm_lcd_read_observer_cb cb, void *context)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    io->read_observer = cb;
    io->read_context = context;
    return STM_OK;
}

stm_err_t stm_lcd_io_deinit(stm_lcd_io_handle_t io)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (io->borrowers || io->complete || io->scanout)
    {
        return STM_ERR_INVALID_STATE;
    }
    memset(io, 0, sizeof(*io));
    return STM_OK;
}

int stm_lcd_io_supports_present(stm_lcd_io_handle_t io)
{
    return io && io->ops && io->ops->present && io->ops->process && io->ops->busy &&
           io->ops->stop_scanout;
}

stm_err_t stm_lcd_io_stop_scanout(stm_lcd_io_handle_t io)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!io->scanout)
    {
        return STM_OK;
    }
    err = io->ops->stop_scanout(io->context);
    if (err == STM_OK)
    {
        io->scanout = 0;
    }
    return err;
}

stm_err_t
stm_lcd_io_present(stm_lcd_io_handle_t io, const void *pixels, uint16_t width, uint16_t height)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!pixels || !width || !height)
    {
        return STM_ERR_INVALID_ARG;
    }
    if ((size_t)width > SIZE_MAX / height / 2u)
    {
        return STM_ERR_OUT_OF_RANGE;
    }
    if (!stm_lcd_io_supports_present(io))
    {
        return STM_ERR_NOT_SUPPORTED;
    }
    io->frame_error = STM_OK;
    io->scanout = 1;
    io->pending = 1;
    err = io->ops->present(io->context, pixels, width, height);
    if (err != STM_OK && io->pending)
    {
        io->frame_error = err;
    }
    if (err != STM_OK && io->pending && !io->ops->busy(io->context))
    {
        stm_err_t stop = io->ops->stop_scanout(io->context);
        if (stop == STM_OK)
        {
            io->scanout = 0;
            io->frame_error = STM_OK;
            io->pending = 0;
        }
    }
    return err;
}

stm_err_t stm_lcd_io_draw_region(
    stm_lcd_io_handle_t io, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const void *pixels)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!pixels || x1 >= x2 || y1 >= y2)
    {
        return STM_ERR_INVALID_ARG;
    }
    if ((size_t)(x2 - x1) > SIZE_MAX / (y2 - y1) / 2u)
    {
        return STM_ERR_OUT_OF_RANGE;
    }
    return io->ops->draw_region ? io->ops->draw_region(io->context, x1, y1, x2, y2, pixels)
                                : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_io_read_reg(
    stm_lcd_io_handle_t io, uint16_t reg, uint8_t address_bytes, uint8_t *data, size_t bytes)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!data || !bytes || (address_bytes != 1 && address_bytes != 2) ||
        (address_bytes == 1 && reg > UINT8_MAX))
    {
        return STM_ERR_INVALID_ARG;
    }
    if (!io->ops->read_reg)
    {
        return STM_ERR_NOT_SUPPORTED;
    }
    err = io->ops->read_reg(io->context, reg, address_bytes, data, bytes);
    if (err == STM_OK && io->read_observer)
    {
        io->dispatching = 1;
        io->read_observer(io->read_context, data, bytes);
        io->dispatching = 0;
    }
    return err;
}

stm_err_t stm_lcd_io_write_reg(
    stm_lcd_io_handle_t io, uint16_t reg, uint8_t address_bytes, const uint8_t *data, size_t bytes)
{
    stm_err_t err = check(io);
    if (err != STM_OK)
    {
        return err;
    }
    if (!data || !bytes || (address_bytes != 1 && address_bytes != 2) ||
        (address_bytes == 1 && reg > UINT8_MAX))
    {
        return STM_ERR_INVALID_ARG;
    }
    return io->ops->write_reg ? io->ops->write_reg(io->context, reg, address_bytes, data, bytes)
                              : STM_ERR_NOT_SUPPORTED;
}
