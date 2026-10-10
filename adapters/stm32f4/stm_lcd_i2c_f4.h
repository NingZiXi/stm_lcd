#ifndef STM_LCD_I2C_F4_H
#define STM_LCD_I2C_F4_H
#include "stm_lcd_impl.h"
#include "stm32f4xx_hal.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct
{
    I2C_HandleTypeDef *i2c; /**< Borrowed, caller initialized. */
    uint8_t address_7bit;   /**< Unshifted address; adapter shifts for HAL. */
    uint32_t timeout_ms;    /**< Nonzero bounded transaction timeout. */
} stm_lcd_i2c_f4_config_t;

typedef struct
{
    struct stm_lcd_io io;           /**< Embedded generic object, caller owned. */
    stm_lcd_i2c_f4_config_t config; /**< Copied configuration. */
} stm_lcd_i2c_f4_t;

stm_err_t stm_lcd_i2c_f4_init(stm_lcd_i2c_f4_t *transport, const stm_lcd_i2c_f4_config_t *config);
stm_err_t stm_lcd_i2c_f4_deinit(stm_lcd_i2c_f4_t *transport);
#ifdef __cplusplus
}
#endif
#endif
