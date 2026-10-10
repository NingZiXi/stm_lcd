/**
 * @file    stm_lcd_panel.c
 * @brief   实现面板操作校验及刷新所有权。
 */
#include "stm_lcd_impl.h"

static void io_complete(void *context, stm_err_t result)
{
    stm_lcd_panel_handle_t p = context;
    if (!p->pending)
        return;
    p->pending = 0;
    p->dispatching = 1;
    if (p->complete)
        p->complete(p->owner, result);
    p->dispatching = 0;
}

stm_err_t stm_lcd_panel_base_init(stm_lcd_panel_handle_t p,
                                  const stm_lcd_panel_ops_t *ops,
                                  stm_lcd_io_handle_t io,
                                  uint16_t w,
                                  uint16_t h)
{
    if (!p || !ops || !io || !io->ops || !w || !h || !ops->destroy)
        return STM_ERR_INVALID_ARG;
    if (p->ops || io->complete || stm_lcd_io_busy(io) || io->dispatching ||
        io->borrowers == UINT16_MAX)
        return STM_ERR_INVALID_STATE;
    p->ops = ops;
    p->io = io;
    p->width = w;
    p->height = h;
    io->complete = io_complete;
    io->owner = p;
    ++io->borrowers;
    return STM_OK;
}

static stm_err_t check(stm_lcd_panel_handle_t p)
{
    if (!p || !p->ops)
        return STM_ERR_INVALID_ARG;
    return p->pending || p->dispatching || stm_lcd_io_busy(p->io) ? STM_ERR_INVALID_STATE : STM_OK;
}

stm_err_t stm_lcd_panel_reset(stm_lcd_panel_handle_t p)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    return p->ops->reset ? p->ops->reset(p) : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_panel_init(stm_lcd_panel_handle_t p)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    return p->ops->init ? p->ops->init(p) : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_panel_display_on_off(stm_lcd_panel_handle_t p, int on)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    if (on != 0 && on != 1)
        return STM_ERR_INVALID_ARG;
    return p->ops->display_on_off ? p->ops->display_on_off(p, on) : STM_ERR_NOT_SUPPORTED;
}

static stm_err_t
rectangle(stm_lcd_panel_handle_t p, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    if (x1 >= x2 || y1 >= y2)
        return STM_ERR_INVALID_ARG;
    if (x2 > p->width || y2 > p->height)
        return STM_ERR_OUT_OF_RANGE;
    if ((size_t)(x2 - x1) > SIZE_MAX / (y2 - y1) / 2u)
        return STM_ERR_OUT_OF_RANGE;
    return STM_OK;
}

stm_err_t stm_lcd_panel_set_window(
    stm_lcd_panel_handle_t p, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    err = rectangle(p, x1, y1, x2, y2);
    if (err != STM_OK)
        return err;
    return p->ops->set_window ? p->ops->set_window(p, x1, y1, x2, y2) : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_panel_draw_bitmap(stm_lcd_panel_handle_t p,
                                    uint16_t x1,
                                    uint16_t y1,
                                    uint16_t x2,
                                    uint16_t y2,
                                    const void *pixels)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    if (!pixels)
        return STM_ERR_INVALID_ARG;
    err = rectangle(p, x1, y1, x2, y2);
    if (err != STM_OK)
        return err;
    return p->ops->draw ? p->ops->draw(p, x1, y1, x2, y2, pixels) : STM_ERR_NOT_SUPPORTED;
}

stm_err_t stm_lcd_panel_draw_bitmap_async(stm_lcd_panel_handle_t p,
                                          uint16_t x1,
                                          uint16_t y1,
                                          uint16_t x2,
                                          uint16_t y2,
                                          const void *pixels)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    if (!pixels)
        return STM_ERR_INVALID_ARG;
    err = rectangle(p, x1, y1, x2, y2);
    if (err != STM_OK)
        return err;
    if (!stm_lcd_panel_supports_async(p))
        return STM_ERR_NOT_SUPPORTED;
    p->pending = 1;
    err = p->ops->draw_async(p, x1, y1, x2, y2, pixels);
    if (err != STM_OK && !stm_lcd_io_busy(p->io))
        p->pending = 0;
    return err;
}

stm_err_t stm_lcd_panel_subscribe(stm_lcd_panel_handle_t p, stm_lcd_complete_cb cb, void *owner)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    if (!cb || !owner)
        return STM_ERR_INVALID_ARG;
    if (p->complete)
        return STM_ERR_INVALID_STATE;
    p->complete = cb;
    p->owner = owner;
    return STM_OK;
}

stm_err_t stm_lcd_panel_unsubscribe(stm_lcd_panel_handle_t p, void *owner)
{
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    if (!owner || p->owner != owner)
        return STM_ERR_INVALID_STATE;
    p->owner = NULL;
    p->complete = NULL;
    return STM_OK;
}

int stm_lcd_panel_busy(stm_lcd_panel_handle_t p)
{
    return p && (p->pending || stm_lcd_io_busy(p->io));
}

int stm_lcd_panel_supports_async(stm_lcd_panel_handle_t p)
{
    return p && p->ops && p->ops->draw_async && stm_lcd_io_supports_async(p->io);
}

stm_lcd_io_handle_t stm_lcd_panel_get_io(stm_lcd_panel_handle_t p)
{
    return p ? p->io : NULL;
}

stm_err_t stm_lcd_panel_get_size(stm_lcd_panel_handle_t p, uint16_t *w, uint16_t *h)
{
    if (!p || !p->ops || !w || !h)
        return STM_ERR_INVALID_ARG;
    *w = p->width;
    *h = p->height;
    return STM_OK;
}

stm_err_t stm_lcd_panel_delete(stm_lcd_panel_handle_t *handle)
{
    if (!handle)
        return STM_ERR_INVALID_ARG;
    stm_lcd_panel_handle_t p = *handle;
    if (!p)
        return STM_OK;
    stm_err_t err = check(p);
    if (err != STM_OK)
        return err;
    if (p->complete || p->io->dispatching)
        return STM_ERR_INVALID_STATE;
    p->io->complete = NULL;
    p->io->owner = NULL;
    --p->io->borrowers;
    p->ops->destroy(p);
    *handle = NULL;
    return STM_OK;
}
