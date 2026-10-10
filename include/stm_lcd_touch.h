/**
 * @file    stm_lcd_touch.h
 * @brief   提供单点快照及统一坐标变换。
 */
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
    uint16_t x, y; // 交换轴、边界检查及镜像后的逻辑坐标。
    uint8_t id;    // 控制器触点 ID，AXS 当前使用零。
} stm_lcd_touch_point_t;

typedef struct
{
    uint16_t x_max, y_max;               // 非零逻辑边界，不包含端点。
    uint8_t swap_xy, mirror_x, mirror_y; // 布尔变换标志。
} stm_lcd_touch_config_t;

/**
 * @brief 复位触摸并清空快照；被输入所有者借用时禁止复位。
 *
 * @param touch 空闲触摸对象
 * @return STM_OK；未实现返回 STM_ERR_NOT_SUPPORTED，被借用时返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_touch_reset(stm_lcd_touch_handle_t touch);

/**
 * @brief 读取并更新单点快照；依次交换轴、检查边界、镜像。
 *
 * @param touch 已创建触摸对象
 * @return STM_OK；无触点或越界时快照为空，读取错误也清空快照。
 */
stm_err_t stm_lcd_touch_read_data(stm_lcd_touch_handle_t touch);

/**
 * @brief 获取快照，不访问总线、不消费触点。
 *
 * @param touch 已创建触摸对象
 * @param points 输出触点数组；capacity 为零时可为空
 * @param capacity 输出容量；当前最多返回一个触点
 * @param count 输出实际触点数
 * @return STM_OK；操作重入返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_touch_get_data(stm_lcd_touch_handle_t touch,
                                 stm_lcd_touch_point_t *points,
                                 size_t capacity,
                                 size_t *count);

/**
 * @brief 获取借用的触摸 IO，不转移所有权。
 *
 * @param touch 待查询触摸对象
 * @return 触摸 IO；空句柄返回 NULL。
 */
stm_lcd_io_handle_t stm_lcd_touch_get_io(stm_lcd_touch_handle_t touch);

/**
 * @brief 独占借用输入对象，防止存活期间复位或删除。
 *
 * @param touch 已创建触摸对象
 * @param owner 非空所有者标识；解除时必须与借用者匹配
 * @return STM_OK；已有所有者或访问中返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_touch_claim(stm_lcd_touch_handle_t touch, void *owner);

/**
 * @brief 解除本输入所有者的借用。
 *
 * @param touch 已创建触摸对象
 * @param owner 非空所有者标识；解除时必须与借用者匹配
 * @return STM_OK；所有者不匹配或访问中返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_touch_release(stm_lcd_touch_handle_t touch, void *owner);

/**
 * @brief 删除触摸并置空句柄，不删除借用 IO。
 *
 * @param touch 句柄地址；空句柄视为已删除
 * @return STM_OK；被借用或访问中返回 STM_ERR_INVALID_STATE。
 */
stm_err_t stm_lcd_touch_delete(stm_lcd_touch_handle_t *touch);
#ifdef __cplusplus
}
#endif
#endif
