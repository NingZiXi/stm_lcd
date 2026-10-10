/**
 * @file    stm_lcd_impl.h
 * @brief   提供驱动实现者使用的嵌入式 base 和操作表。
 */
#ifndef STM_LCD_IMPL_H
#define STM_LCD_IMPL_H
#include "stm_lcd.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct
{
    stm_err_t (*tx_param)(void *, uint8_t, const uint8_t *, size_t); // 可选同步命令事务。
    stm_err_t (*tx_color)(void *, uint8_t, const void *, size_t);    // 可选同步像素事务。
    stm_err_t (*tx_color_async)(void *, uint8_t, const void *,
                                size_t);                 // 硬件停止前保留像素。
    stm_err_t (*write)(void *, const uint8_t *, size_t); // 独立原始 I2C 写事务，结束产生 STOP。
    stm_err_t (*read)(void *, uint8_t *, size_t);        // 独立原始 I2C 读事务。
    stm_err_t (*probe)(void *);                          // 可选探测，不复位设备。
    void (*process)(void *);                             // 主循环完成及超时服务。
    int (*busy)(void *);                                 // 硬件访问保护；停止后返回零。
} stm_lcd_io_ops_t;

struct stm_lcd_io
{
    const stm_lcd_io_ops_t *ops;            // 操作表须比 IO 存活更久。
    void *context;                          // 借用的实现上下文。
    stm_lcd_complete_cb complete;           // 唯一设备完成订阅者。
    void *owner;                            // 订阅者标识。
    stm_lcd_read_observer_cb read_observer; // 只读诊断观察者。
    void *read_context;                     // 借用的观察上下文。
    uint16_t borrowers;                     // 设备借用计数，非零时禁止清除 IO。
    uint8_t pending, dispatching;           // 在途及回调保护，仅主循环访问。
};

typedef struct
{
    stm_err_t (*reset)(stm_lcd_panel_handle_t);               // 可选硬件复位。
    stm_err_t (*init)(stm_lcd_panel_handle_t);                // 控制器初始化。
    stm_err_t (*display_on_off)(stm_lcd_panel_handle_t, int); // 可选显示开关。
    stm_err_t (*set_window)(
        stm_lcd_panel_handle_t, uint16_t, uint16_t, uint16_t, uint16_t); // 可选窗口设置。
    stm_err_t (*draw)(
        stm_lcd_panel_handle_t, uint16_t, uint16_t, uint16_t, uint16_t, const void *); // 同步绘图。
    stm_err_t (*draw_async)(stm_lcd_panel_handle_t,
                            uint16_t,
                            uint16_t,
                            uint16_t,
                            uint16_t,
                            const void *);   // 安全完成前借用像素。
    void (*destroy)(stm_lcd_panel_handle_t); // 仅释放实现对象，不删除 IO。
} stm_lcd_panel_ops_t;

struct stm_lcd_panel
{
    const stm_lcd_panel_ops_t *ops; // 持续有效的实现操作表。
    stm_lcd_io_handle_t io;         // 借用的 IO。
    uint16_t width, height;         // 逻辑尺寸。
    stm_lcd_complete_cb complete;   // 独占刷新订阅者。
    void *owner;                    // 订阅者标识。
    uint8_t pending, dispatching;   // 完成与删除保护。
};

typedef struct
{
    stm_err_t (*reset)(stm_lcd_touch_handle_t); // 可选启动复位。
    stm_err_t (*read_data)(stm_lcd_touch_handle_t,
                           stm_lcd_touch_point_t *,
                           size_t *);        // 解析一个原始触点，不进行坐标变换。
    void (*destroy)(stm_lcd_touch_handle_t); // 仅释放实现对象。
} stm_lcd_touch_ops_t;

struct stm_lcd_touch
{
    const stm_lcd_touch_ops_t *ops; // 持续有效的实现操作表。
    stm_lcd_io_handle_t io;         // 借用的 IO。
    stm_lcd_touch_config_t config;  // 统一坐标变换配置。
    stm_lcd_touch_point_t point;    // 非消费逻辑触点快照。
    void *owner;                    // 输入所有者，借用期间禁止删除。
    uint8_t count, accessing;       // 快照有效性及访问保护。
};

/**
 * @brief 初始化实现者提供的已清零 IO base。
 *
 * @param io 由实现者持有的清零对象
 * @param ops 持续有效的操作表
 * @param context 持续有效的实现上下文
 * @return STM_OK；重复初始化返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_io_init(stm_lcd_io_handle_t io, const stm_lcd_io_ops_t *ops, void *context);

/**
 * @brief 硬件已停止访问后在主循环通知一次完成。
 *
 * @param io 存在待完成请求的 IO
 * @param result 传输最终结果
 * @return STM_OK；重复、异常或硬件仍忙的完成返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_io_complete(stm_lcd_io_handle_t io, stm_err_t result);

/**
 * @brief 初始化嵌入驱动的清零面板 base，并借用 IO。
 *
 * @param panel 嵌入实现对象的清零 base
 * @param ops 持续有效的操作表，必须提供 destroy
 * @param io 已初始化且未被其他面板订阅的 IO
 * @param width 非零逻辑宽度
 * @param height 非零逻辑高度
 * @return STM_OK；无效参数或 IO 所有权冲突时失败。
 */
stm_err_t stm_lcd_panel_base_init(stm_lcd_panel_handle_t panel,
                                  const stm_lcd_panel_ops_t *ops,
                                  stm_lcd_io_handle_t io,
                                  uint16_t width,
                                  uint16_t height);

/**
 * @brief 初始化嵌入驱动的清零触摸 base，并复制坐标配置。
 *
 * @param touch 嵌入实现对象的清零 base
 * @param ops 持续有效的操作表，必须提供 read_data 和 destroy
 * @param io 借用的已初始化 IO
 * @param config 非零逻辑边界及布尔变换配置
 * @return STM_OK；配置无效或借用状态冲突时失败。
 */
stm_err_t stm_lcd_touch_base_init(stm_lcd_touch_handle_t touch,
                                  const stm_lcd_touch_ops_t *ops,
                                  stm_lcd_io_handle_t io,
                                  const stm_lcd_touch_config_t *config);
#ifdef __cplusplus
}
#endif
#endif
