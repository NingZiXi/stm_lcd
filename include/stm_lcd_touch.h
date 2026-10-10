/** @file stm_lcd_touch.h @brief Single-point non-consuming snapshots and generic coordinates. */
#ifndef STM_LCD_TOUCH_H
#define STM_LCD_TOUCH_H
#include "stm_lcd_io.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct stm_lcd_touch *stm_lcd_touch_handle_t;

typedef struct
{
    uint16_t x, y; /**< Logical coordinates after swap, bounds, mirror, once. */
    uint8_t id;    /**< Controller-provided ID; AXS uses zero. */
} stm_lcd_touch_point_t;

typedef struct
{
    uint16_t x_max, y_max;               /**< Nonzero logical exclusive bounds. */
    uint8_t swap_xy, mirror_x, mirror_y; /**< Boolean transform flags. */
} stm_lcd_touch_config_t;

stm_err_t stm_lcd_touch_reset(stm_lcd_touch_handle_t touch);
stm_err_t stm_lcd_touch_read_data(stm_lcd_touch_handle_t touch);
/* Snapshot only: no bus access and no consumption. Current generic service stores one point. */
stm_err_t stm_lcd_touch_get_data(stm_lcd_touch_handle_t touch,
                                 stm_lcd_touch_point_t *points,
                                 size_t capacity,
                                 size_t *count);
stm_lcd_io_handle_t stm_lcd_touch_get_io(stm_lcd_touch_handle_t touch);
stm_err_t stm_lcd_touch_claim(stm_lcd_touch_handle_t touch, void *owner);
stm_err_t stm_lcd_touch_release(stm_lcd_touch_handle_t touch, void *owner);
stm_err_t stm_lcd_touch_delete(stm_lcd_touch_handle_t *touch);
#ifdef __cplusplus
}
#endif
#endif
