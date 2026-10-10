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

/**
 * @brief 同步复制紧密 RGB565 区域至平台帧缓冲，返回后可复用输入。
 * @param io 空闲 IO
 * @param x1 左边界
 * @param y1 上边界
 * @param x2 不包含的右边界
 * @param y2 不包含的下边界
 * @param pixels 紧密排列的区域像素
 * @return STM_OK；缺少能力返回 NOT_SUPPORTED，平台错误原样传递。
 */
stm_err_t stm_lcd_io_draw_region(
    stm_lcd_io_handle_t io, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const void *pixels);
/**
 * @brief 提交完整紧密 RGB565 帧；安全 VSYNC 完成后释放旧扫描帧，新帧继续借用。
 * @param io 实现 present/process/busy/stop_scanout 的 IO
 * @param pixels 整帧首地址，成功切帧后仍禁止修改，直至下次切帧或停止扫描
 * @param width 帧宽
 * @param height 帧高
 * @return STM_OK 或提交错误；错误且忙时保持借用，必须继续 process。
 */
stm_err_t
stm_lcd_io_present(stm_lcd_io_handle_t io, const void *pixels, uint16_t width, uint16_t height);
/**
 * @brief 查询整帧提交与安全停止能力。
 * @param io IO 句柄
 * @return 非零表示支持完整扫描契约。
 */
int stm_lcd_io_supports_present(stm_lcd_io_handle_t io);
/**
 * @brief 空闲时停止扫描并解除帧缓冲借用；失败继续保护缓冲。
 * @param io IO 句柄
 * @return STM_OK 或平台停止错误；在途时返回 INVALID_STATE。
 */
stm_err_t stm_lcd_io_stop_scanout(stm_lcd_io_handle_t io);
/**
 * @brief 原子寄存器读，事务形式由平台保证，不假设 raw write/read 能合并。
 * @param io IO 句柄
 * @param reg 寄存器地址
 * @param address_bytes 地址宽度，1 或 2
 * @param data 输出数据
 * @param bytes 非零数据字节数
 * @return STM_OK 或参数、能力、平台错误。
 */
stm_err_t stm_lcd_io_read_reg(
    stm_lcd_io_handle_t io, uint16_t reg, uint8_t address_bytes, uint8_t *data, size_t bytes);
/**
 * @brief 原子寄存器写。
 * @param io IO 句柄
 * @param reg 寄存器地址
 * @param address_bytes 地址宽度，1 或 2
 * @param data 输入数据
 * @param bytes 非零数据字节数
 * @return STM_OK 或参数、能力、平台错误。
 */
stm_err_t stm_lcd_io_write_reg(
    stm_lcd_io_handle_t io, uint16_t reg, uint8_t address_bytes, const uint8_t *data, size_t bytes);

#ifdef __cplusplus
}
#endif
#endif
