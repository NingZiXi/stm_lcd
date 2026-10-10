/**
 * @file    stm_lcd_io.h
 * @brief   提供同步及异步显示 IO 接口。
 */
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

/**
 * @brief 同步发送命令和参数；未实现时返回 NOT_SUPPORTED。
 *
 * @param io 已初始化的 IO
 * @param command 控制器命令字节
 * @param data 参数字节，可在 bytes 为零时为空
 * @param bytes 传输字节数
 * @return STM_OK 或参数、状态、能力及传输错误。
 */
stm_err_t
stm_lcd_io_tx_param(stm_lcd_io_handle_t io, uint8_t command, const uint8_t *data, size_t bytes);

/**
 * @brief 同步发送像素，返回后可复用缓冲。
 *
 * @param io 已初始化的 IO
 * @param command 控制器命令字节
 * @param data 已准备好字节序的像素缓冲
 * @param bytes 传输字节数
 * @return STM_OK 或参数、状态、能力及传输错误。
 */
stm_err_t
stm_lcd_io_tx_color(stm_lcd_io_handle_t io, uint8_t command, const void *data, size_t bytes);

/**
 * @brief 提交异步像素传输；完成前不得复用缓冲。
 *
 * @param io 已初始化的 IO
 * @param command 控制器命令字节
 * @param data 借用至安全完成的像素缓冲
 * @param bytes 传输字节数
 * @return STM_OK 表示成功；异步表示已接受，失败但 IO 忙时仍须服务完成。
 */
stm_err_t
stm_lcd_io_tx_color_async(stm_lcd_io_handle_t io, uint8_t command, const void *data, size_t bytes);

/**
 * @brief 执行独立原始 I2C 写事务，事务结束产生 STOP。
 *
 * @param io 已初始化的 IO
 * @param data 待发送字节
 * @param bytes 非零传输字节数
 * @return STM_OK 或参数、状态、能力及通信错误。
 */
stm_err_t stm_lcd_io_write(stm_lcd_io_handle_t io, const uint8_t *data, size_t bytes);

/**
 * @brief 执行独立原始 I2C 读事务，成功后通知读取观察者。
 *
 * @param io 已初始化的 IO
 * @param data 接收缓冲
 * @param bytes 非零传输字节数
 * @return STM_OK 或参数、状态、能力及通信错误。
 */
stm_err_t stm_lcd_io_read(stm_lcd_io_handle_t io, uint8_t *data, size_t bytes);

/**
 * @brief 探测连接，不复位设备。
 *
 * @param io 已初始化的 IO
 * @return STM_OK 表示在线；未实现返回 STM_ERR_NOT_SUPPORTED。
 */
stm_err_t stm_lcd_io_probe(stm_lcd_io_handle_t io);

/**
 * @brief 在主循环服务异步完成和超时，不在 ISR 调用。
 *
 * @param io 待服务 IO；空句柄不执行操作
 */
void stm_lcd_io_process(stm_lcd_io_handle_t io);

/**
 * @brief 查询请求或硬件是否仍占用 IO。
 *
 * @param io 待查询 IO
 * @return 非零表示仍忙；空句柄返回零。
 */
int stm_lcd_io_busy(stm_lcd_io_handle_t io);

/**
 * @brief 查询 IO 是否同时实现异步提交和主循环服务。
 *
 * @param io 待查询 IO
 * @return 非零表示支持；空句柄返回零。
 */
int stm_lcd_io_supports_async(stm_lcd_io_handle_t io);

/**
 * @brief 设置读取观察者；回调中禁止递归访问或删除对象。
 *
 * @param io 空闲 IO
 * @param callback 观察回调；NULL 解除观察
 * @param context 借用的观察上下文
 * @return STM_OK；忙或回调期间返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_io_set_read_observer(stm_lcd_io_handle_t io,
                                       stm_lcd_read_observer_cb callback,
                                       void *context);

/**
 * @brief 清除 IO 契约，不销毁调用方的硬件资源。
 *
 * @param io 待清除 IO
 * @return STM_OK；存在借用者、订阅者或在途操作返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_io_deinit(stm_lcd_io_handle_t io);
#ifdef __cplusplus
}
#endif
#endif
