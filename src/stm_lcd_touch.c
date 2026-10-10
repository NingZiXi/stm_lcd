/**
 * @file    stm_lcd_touch.c
 * @brief   实现触摸快照、坐标变换及输入借用。
 */
#include "stm_lcd_impl.h"

stm_err_t stm_lcd_touch_base_init(stm_lcd_touch_handle_t t,
                                  const stm_lcd_touch_ops_t *ops,
                                  stm_lcd_io_handle_t io,
                                  const stm_lcd_touch_config_t *c)
{
    if (!t || !ops || !ops->read_data || !ops->destroy || !io || !io->ops || !c)
        return STM_ERR_INVALID_ARG;
    if (!c->x_max || !c->y_max || c->swap_xy > 1 || c->mirror_x > 1 || c->mirror_y > 1)
        return STM_ERR_INVALID_CONFIG;
    if (t->ops || io->dispatching || io->borrowers == UINT16_MAX)
        return STM_ERR_INVALID_STATE;
    t->ops = ops;
    t->io = io;
    t->config = *c;
    ++io->borrowers;
    return STM_OK;
}

stm_err_t stm_lcd_touch_reset(stm_lcd_touch_handle_t t)
{
    if (!t || !t->ops)
        return STM_ERR_INVALID_ARG;
    if (t->accessing || t->owner)
        return STM_ERR_INVALID_STATE;
    t->count = 0;
    t->accessing = 1;
    stm_err_t err = t->ops->reset ? t->ops->reset(t) : STM_ERR_NOT_SUPPORTED;
    t->accessing = 0;
    return err;
}

stm_err_t stm_lcd_touch_read_data(stm_lcd_touch_handle_t t)
{
    if (!t || !t->ops)
        return STM_ERR_INVALID_ARG;
    if (t->accessing)
        return STM_ERR_INVALID_STATE;
    t->accessing = 1;
    t->count = 0;
    stm_lcd_touch_point_t p = {0};
    size_t count = 0;
    stm_err_t err = t->ops->read_data(t, &p, &count);
    if (err == STM_OK && count > 1)
        err = STM_ERR_VERIFY;
    if (err == STM_OK && count)
    {
        if (t->config.swap_xy)
        {
            uint16_t temp = p.x;
            p.x = p.y;
            p.y = temp;
        }
        if (p.x < t->config.x_max && p.y < t->config.y_max)
        {
            if (t->config.mirror_x)
                p.x = t->config.x_max - 1u - p.x;
            if (t->config.mirror_y)
                p.y = t->config.y_max - 1u - p.y;
            t->point = p;
            t->count = 1;
        }
    }
    t->accessing = 0;
    return err;
}

stm_err_t stm_lcd_touch_get_data(stm_lcd_touch_handle_t t,
                                 stm_lcd_touch_point_t *points,
                                 size_t capacity,
                                 size_t *count)
{
    if (count)
        *count = 0;
    if (!t || !t->ops || !count || (capacity && !points))
        return STM_ERR_INVALID_ARG;
    if (t->accessing)
        return STM_ERR_INVALID_STATE;
    if (capacity && t->count)
    {
        *points = t->point;
        *count = 1;
    }
    return STM_OK;
}

stm_lcd_io_handle_t stm_lcd_touch_get_io(stm_lcd_touch_handle_t t)
{
    return t ? t->io : NULL;
}

stm_err_t stm_lcd_touch_claim(stm_lcd_touch_handle_t t, void *owner)
{
    if (!t || !t->ops || !owner)
        return STM_ERR_INVALID_ARG;
    if (t->owner || t->accessing)
        return STM_ERR_INVALID_STATE;
    t->owner = owner;
    return STM_OK;
}

stm_err_t stm_lcd_touch_release(stm_lcd_touch_handle_t t, void *owner)
{
    if (!t || !t->ops)
        return STM_ERR_INVALID_ARG;
    if (t->accessing || !owner || t->owner != owner)
        return STM_ERR_INVALID_STATE;
    t->owner = NULL;
    return STM_OK;
}

stm_err_t stm_lcd_touch_delete(stm_lcd_touch_handle_t *handle)
{
    if (!handle)
        return STM_ERR_INVALID_ARG;
    stm_lcd_touch_handle_t t = *handle;
    if (!t)
        return STM_OK;
    if (t->accessing || t->owner || t->io->dispatching)
        return STM_ERR_INVALID_STATE;
    --t->io->borrowers;
    t->ops->destroy(t);
    *handle = NULL;
    return STM_OK;
}
