/**
 * @file    stm_lcd_i2c_f4.h
 * @brief   提供可选 STM32F4 原始 I2C 适配接口。
 */
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
    I2C_HandleTypeDef *i2c; // 借用的已初始化 HAL 句柄。
    uint8_t address_7bit;   // 未左移的 7 位地址，由适配层左移。
    uint32_t timeout_ms;    // 非零且不超过 INT32_MAX 的事务超时。
} stm_lcd_i2c_f4_config_t;

typedef struct
{
    struct stm_lcd_io io;           // 调用方持有的嵌入式 IO base。
    stm_lcd_i2c_f4_config_t config; // 配置副本。
} stm_lcd_i2c_f4_t;

/**
 * @brief 初始化调用方持有的清零 STM32F4 I2C 适配，不初始化硬件。
 *
 * @param transport 持续有效的已清零适配对象
 * @param config 已初始化 HAL 句柄及有效配置；复制后不修改
 * @return STM_OK；参数、配置或重复初始化错误时失败。
 */
stm_err_t stm_lcd_i2c_f4_init(stm_lcd_i2c_f4_t *transport, const stm_lcd_i2c_f4_config_t *config);

/**
 * @brief 清除适配对象，不销毁 HAL 外设。
 *
 * @param transport 待清除适配对象
 * @return STM_OK；有借用者或在途操作时返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_i2c_f4_deinit(stm_lcd_i2c_f4_t *transport);
#ifdef __cplusplus
}
#endif
#endif
