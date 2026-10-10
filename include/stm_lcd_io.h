/** @file stm_lcd_io.h @brief Caller-owned display IO; main-loop serialized calls only. */
#ifndef STM_LCD_IO_H
#define STM_LCD_IO_H
#include "stm_err.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct stm_lcd_io *stm_lcd_io_handle_t;
typedef void (*stm_lcd_complete_cb)(void *context, stm_err_t result);
typedef void (*stm_lcd_read_observer_cb)(void *context, const uint8_t *data, size_t bytes);

/* Devices borrow IO. Optional methods return STM_ERR_NOT_SUPPORTED. */
stm_err_t
stm_lcd_io_tx_param(stm_lcd_io_handle_t io, uint8_t command, const uint8_t *data, size_t bytes);
stm_err_t
stm_lcd_io_tx_color(stm_lcd_io_handle_t io, uint8_t command, const void *data, size_t bytes);
stm_err_t
stm_lcd_io_tx_color_async(stm_lcd_io_handle_t io, uint8_t command, const void *data, size_t bytes);
stm_err_t stm_lcd_io_write(stm_lcd_io_handle_t io, const uint8_t *data, size_t bytes);
stm_err_t stm_lcd_io_read(stm_lcd_io_handle_t io, uint8_t *data, size_t bytes);
stm_err_t stm_lcd_io_probe(stm_lcd_io_handle_t io);
void stm_lcd_io_process(stm_lcd_io_handle_t io);
int stm_lcd_io_busy(stm_lcd_io_handle_t io);
int stm_lcd_io_supports_async(stm_lcd_io_handle_t io);
stm_err_t stm_lcd_io_set_read_observer(stm_lcd_io_handle_t io,
                                       stm_lcd_read_observer_cb callback,
                                       void *context);
stm_err_t stm_lcd_io_deinit(stm_lcd_io_handle_t io);
#ifdef __cplusplus
}
#endif
#endif
