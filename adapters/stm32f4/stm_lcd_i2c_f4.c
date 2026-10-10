#include "stm_lcd_i2c_f4.h"
#include <limits.h>
#include <string.h>

static stm_err_t from_hal(HAL_StatusTypeDef result)
{
    return result == HAL_OK ? STM_OK : result == HAL_TIMEOUT ? STM_ERR_TIMEOUT : STM_ERR_IO;
}

static stm_err_t write(void *context, const uint8_t *data, size_t n)
{
    stm_lcd_i2c_f4_t *t = context;
    if (n > UINT16_MAX)
        return STM_ERR_OUT_OF_RANGE;
    return from_hal(HAL_I2C_Master_Transmit(t->config.i2c, t->config.address_7bit << 1,
                                            (uint8_t *)data, (uint16_t)n, t->config.timeout_ms));
}

static stm_err_t read(void *context, uint8_t *data, size_t n)
{
    stm_lcd_i2c_f4_t *t = context;
    if (n > UINT16_MAX)
        return STM_ERR_OUT_OF_RANGE;
    /* Two independent raw transactions with STOP, never HAL_I2C_Mem_Read. */
    return from_hal(HAL_I2C_Master_Receive(t->config.i2c, t->config.address_7bit << 1, data,
                                           (uint16_t)n, t->config.timeout_ms));
}

static stm_err_t probe(void *context)
{
    stm_lcd_i2c_f4_t *t = context;
    return from_hal(
        HAL_I2C_IsDeviceReady(t->config.i2c, t->config.address_7bit << 1, 1, t->config.timeout_ms));
}

static const stm_lcd_io_ops_t ops = {.write = write, .read = read, .probe = probe};

stm_err_t stm_lcd_i2c_f4_init(stm_lcd_i2c_f4_t *t, const stm_lcd_i2c_f4_config_t *c)
{
    if (!t || !c)
        return STM_ERR_INVALID_ARG;
    if (t->io.ops)
        return STM_ERR_INVALID_STATE;
    if (!c->i2c || c->address_7bit > 0x7f || !c->timeout_ms || c->timeout_ms > INT32_MAX)
        return STM_ERR_INVALID_CONFIG;
    stm_err_t err = stm_lcd_io_init(&t->io, &ops, t);
    if (err == STM_OK)
        t->config = *c;
    return err;
}

stm_err_t stm_lcd_i2c_f4_deinit(stm_lcd_i2c_f4_t *t)
{
    if (!t)
        return STM_ERR_INVALID_ARG;
    stm_err_t err = stm_lcd_io_deinit(&t->io);
    if (err == STM_OK)
        memset(t, 0, sizeof(*t));
    return err;
}
