/** @file stm_lcd_panel.h @brief Borrowed IO and exclusive refresh ownership. */
#ifndef STM_LCD_PANEL_H
#define STM_LCD_PANEL_H
#include "stm_lcd_io.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct stm_lcd_panel *stm_lcd_panel_handle_t;

/* Right/bottom draw edges are exclusive. Deletion never destroys borrowed IO. */
stm_err_t stm_lcd_panel_reset(stm_lcd_panel_handle_t panel);
stm_err_t stm_lcd_panel_init(stm_lcd_panel_handle_t panel);
stm_err_t stm_lcd_panel_display_on_off(stm_lcd_panel_handle_t panel, int on);
stm_err_t stm_lcd_panel_set_window(
    stm_lcd_panel_handle_t panel, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
stm_err_t stm_lcd_panel_draw_bitmap(stm_lcd_panel_handle_t panel,
                                    uint16_t x1,
                                    uint16_t y1,
                                    uint16_t x2,
                                    uint16_t y2,
                                    const void *pixels);
stm_err_t stm_lcd_panel_draw_bitmap_async(stm_lcd_panel_handle_t panel,
                                          uint16_t x1,
                                          uint16_t y1,
                                          uint16_t x2,
                                          uint16_t y2,
                                          const void *pixels);
/* One refresh owner. Same owner cannot silently replace an existing subscription.
 * Unsubscribe and delete reject pending transfers or completion dispatch. */
stm_err_t
stm_lcd_panel_subscribe(stm_lcd_panel_handle_t panel, stm_lcd_complete_cb callback, void *owner);
stm_err_t stm_lcd_panel_unsubscribe(stm_lcd_panel_handle_t panel, void *owner);
int stm_lcd_panel_busy(stm_lcd_panel_handle_t panel);
int stm_lcd_panel_supports_async(stm_lcd_panel_handle_t panel);
stm_lcd_io_handle_t stm_lcd_panel_get_io(stm_lcd_panel_handle_t panel);
stm_err_t stm_lcd_panel_get_size(stm_lcd_panel_handle_t panel, uint16_t *width, uint16_t *height);
stm_err_t stm_lcd_panel_delete(stm_lcd_panel_handle_t *panel);
#ifdef __cplusplus
}
#endif
#endif
