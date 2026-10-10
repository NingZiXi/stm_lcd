/**
 * @file    stm_lcd_panel.h
 * @brief   提供通用面板操作及独占刷新订阅。
 */
#ifndef STM_LCD_PANEL_H
#define STM_LCD_PANEL_H
#include "stm_lcd_io.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct stm_lcd_panel *stm_lcd_panel_handle_t;

/**
 * @brief 执行面板复位。
 *
 * @param panel 空闲面板
 * @return STM_OK；未实现返回 STM_ERR_NOT_SUPPORTED，其他错误由驱动返回。
 */
stm_err_t stm_lcd_panel_reset(stm_lcd_panel_handle_t panel);

/**
 * @brief 执行控制器初始化。
 *
 * @param panel 空闲面板
 * @return STM_OK；未实现返回 STM_ERR_NOT_SUPPORTED，其他错误由驱动返回。
 */
stm_err_t stm_lcd_panel_init(stm_lcd_panel_handle_t panel);

/**
 * @brief 设置面板显示开关，不管理板级背光。
 *
 * @param panel 空闲面板
 * @param on 1 开启，0 关闭
 * @return STM_OK 或参数、状态、能力及通信错误。
 */
stm_err_t stm_lcd_panel_display_on_off(stm_lcd_panel_handle_t panel, int on);

/**
 * @brief 设置非空且位于面板范围内的窗口。
 *
 * @param panel 空闲面板
 * @param x1 包含的左边界
 * @param y1 包含的上边界
 * @param x2 不包含的右边界
 * @param y2 不包含的下边界
 * @return STM_OK；越界返回 STM_ERR_OUT_OF_RANGE，未实现返回 STM_ERR_NOT_SUPPORTED。
 */
stm_err_t stm_lcd_panel_set_window(
    stm_lcd_panel_handle_t panel, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief 同步发送矩形 RGB565 像素，返回后可复用缓冲。
 *
 * @param panel 空闲面板
 * @param x1 包含的左边界
 * @param y1 包含的上边界
 * @param x2 不包含的右边界
 * @param y2 不包含的下边界
 * @param pixels 至少覆盖矩形的连续像素缓冲，字节序由调用方准备
 * @return STM_OK 或参数、越界、状态、能力及传输错误。
 */
stm_err_t stm_lcd_panel_draw_bitmap(stm_lcd_panel_handle_t panel,
                                    uint16_t x1,
                                    uint16_t y1,
                                    uint16_t x2,
                                    uint16_t y2,
                                    const void *pixels);

/**
 * @brief 异步发送矩形 RGB565 像素，安全完成前借用缓冲。
 *
 * @param panel 空闲面板
 * @param x1 包含的左边界
 * @param y1 包含的上边界
 * @param x2 不包含的右边界
 * @param y2 不包含的下边界
 * @param pixels 至少覆盖矩形的连续像素缓冲，字节序由调用方准备
 * @return STM_OK 表示成功；异步表示已接受，失败且面板忙时不得释放缓冲。
 */
stm_err_t stm_lcd_panel_draw_bitmap_async(stm_lcd_panel_handle_t panel,
                                          uint16_t x1,
                                          uint16_t y1,
                                          uint16_t x2,
                                          uint16_t y2,
                                          const void *pixels);

/**
 * @brief 独占订阅刷新完成，不允许覆盖已有订阅。
 *
 * @param panel 空闲面板
 * @param callback 必需的主循环完成回调
 * @param owner 非空所有者标识，同时作为回调上下文
 * @return STM_OK；已有订阅或在途操作返回 STM_ERR_INVALID_STATE。
 */
stm_err_t
stm_lcd_panel_subscribe(stm_lcd_panel_handle_t panel, stm_lcd_complete_cb callback, void *owner);

/**
 * @brief 解除本所有者的刷新订阅。
 *
 * @param panel 空闲面板
 * @param owner 必须匹配订阅者标识
 * @return STM_OK；所有者不匹配、在途或回调期间返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_panel_unsubscribe(stm_lcd_panel_handle_t panel, void *owner);

/**
 * @brief 查询面板或其 IO 是否仍占用像素。
 *
 * @param panel 待查询面板
 * @return 非零表示忙；空句柄返回零。
 */
int stm_lcd_panel_busy(stm_lcd_panel_handle_t panel);

/**
 * @brief 查询面板和 IO 是否均支持异步绘图。
 *
 * @param panel 待查询面板
 * @return 非零表示支持；空句柄返回零。
 */
int stm_lcd_panel_supports_async(stm_lcd_panel_handle_t panel);

/**
 * @brief 获取借用的 IO，不转移所有权。
 *
 * @param panel 待查询面板
 * @return 面板 IO；空句柄返回 NULL。
 */
stm_lcd_io_handle_t stm_lcd_panel_get_io(stm_lcd_panel_handle_t panel);

/**
 * @brief 获取面板逻辑尺寸。
 *
 * @param panel 已创建面板
 * @param width 输出宽度
 * @param height 输出高度
 * @return STM_OK；空参数返回 STM_ERR_INVALID_ARG。
 */
stm_err_t stm_lcd_panel_get_size(stm_lcd_panel_handle_t panel, uint16_t *width, uint16_t *height);

/**
 * @brief 删除面板并置空句柄，不删除借用 IO。
 *
 * @param panel 句柄地址；空句柄视为已删除
 * @return STM_OK；在途、回调或仍被订阅时返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_panel_delete(stm_lcd_panel_handle_t *panel);

/**
 * @brief 查询面板同步区域绘图能力。
 * @param panel 面板句柄
 * @return 非零表示实现同步 draw。
 */
int stm_lcd_panel_supports_draw(stm_lcd_panel_handle_t panel);
/**
 * @brief 查询面板安全整帧提交能力。
 * @param panel 面板句柄
 * @return 非零表示支持 present 与 IO 安全停止。
 */
int stm_lcd_panel_supports_present(stm_lcd_panel_handle_t panel);
/**
 * @brief 提交完整 RGB565 帧，异步完成时旧扫描帧可复用，新帧仍被扫描。
 * @param panel 空闲且已初始化面板
 * @param pixels 面板尺寸的整帧首地址
 * @return STM_OK 或错误；忙时必须继续完成服务，禁止复用或删除。
 */
stm_err_t stm_lcd_panel_present(stm_lcd_panel_handle_t panel, const void *pixels);
/**
 * @brief 空闲时安全停止扫描，解除全部扫描缓冲借用。
 * @param panel 面板句柄
 * @return STM_OK 或平台停止错误，失败不解除借用。
 */
stm_err_t stm_lcd_panel_stop_scanout(stm_lcd_panel_handle_t panel);

#ifdef __cplusplus
}
#endif
#endif
