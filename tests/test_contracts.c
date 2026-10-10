/**
 * @file    test_contracts.c
 * @brief   验证组件主机回归，不替代实板验收。
 */
#include "stm_lcd_impl.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static struct stm_lcd_io io;
static struct stm_lcd_panel panel;
static struct stm_lcd_touch touch;
static int hardware_busy, inline_done, bad_submit, unsafe_submit;
static unsigned completions, reads, destroys, observers;
static stm_err_t result, read_result;
static size_t raw_count;
static stm_lcd_touch_point_t raw;

static int busy(void *c)
{
    (void)c;
    return hardware_busy;
}

static stm_err_t submit(void *c, uint8_t cmd, const void *data, size_t bytes)
{
    (void)c;
    (void)cmd;
    assert(data && bytes == 2);
    if (unsafe_submit)
        hardware_busy = 1;
    if (inline_done)
        assert(stm_lcd_io_complete(&io, STM_ERR_IO) == STM_OK);
    return bad_submit ? STM_ERR_IO : STM_OK;
}

static void process(void *c)
{
    (void)c;
}

static stm_err_t read_io(void *c, uint8_t *data, size_t bytes)
{
    (void)c;
    memset(data, 0, bytes);
    return STM_OK;
}

static void observe(void *c, const uint8_t *data, size_t bytes)
{
    (void)c;
    assert(data && bytes == 1);
    ++observers;
    assert(stm_lcd_io_deinit(&io) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_io_set_read_observer(&io, NULL, NULL) == STM_ERR_INVALID_STATE);
}

static void complete(void *c, stm_err_t err)
{
    assert(c == &result);
    result = err;
    ++completions;
    assert(stm_lcd_io_complete(&io, STM_OK) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_unsubscribe(&panel, &result) == STM_ERR_INVALID_STATE);
    stm_lcd_panel_handle_t alias = &panel;
    assert(stm_lcd_panel_delete(&alias) == STM_ERR_INVALID_STATE);
}

static stm_err_t draw_async(stm_lcd_panel_handle_t p,
                            uint16_t x1,
                            uint16_t y1,
                            uint16_t x2,
                            uint16_t y2,
                            const void *pixels)
{
    assert(x1 == 0 && y1 == 0 && x2 == 1 && y2 == 1);
    return stm_lcd_io_tx_color_async(p->io, 0x2c, pixels, 2);
}

static stm_err_t read_touch(stm_lcd_touch_handle_t t, stm_lcd_touch_point_t *p, size_t *count)
{
    (void)t;
    ++reads;
    *p = raw;
    *count = raw_count;
    return read_result;
}

static void destroy_panel(stm_lcd_panel_handle_t p)
{
    assert(p == &panel);
    ++destroys;
}

static void destroy_touch(stm_lcd_touch_handle_t t)
{
    assert(t == &touch);
    ++destroys;
}

static const stm_lcd_io_ops_t io_ops = {
    .tx_color_async = submit, .process = process, .busy = busy, .read = read_io};
static const stm_lcd_panel_ops_t panel_ops = {.draw_async = draw_async, .destroy = destroy_panel};
static const stm_lcd_touch_ops_t touch_ops = {.read_data = read_touch, .destroy = destroy_touch};

int main(void)
{
    uint16_t pixel = 0;
    assert(stm_lcd_io_init(NULL, &io_ops, NULL) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_io_init(&io, &io_ops, NULL) == STM_OK);
    assert(stm_lcd_io_init(&io, &io_ops, NULL) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_io_tx_param(&io, 0, NULL, 0) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_probe(&io) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_write(&io, (uint8_t *)&pixel, 1) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_tx_color(&io, 0, &pixel, 2) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_complete(&io, STM_OK) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_base_init(&panel, &panel_ops, &io, 170, 560) == STM_OK);
    assert(stm_lcd_io_deinit(&io) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_reset(&panel) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_panel_init(&panel) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_panel_display_on_off(&panel, 1) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_panel_set_window(&panel, 0, 0, 1, 1) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_panel_draw_bitmap(&panel, 0, 0, 1, 1, &pixel) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_panel_draw_bitmap_async(&panel, 0, 0, 171, 1, &pixel) == STM_ERR_OUT_OF_RANGE);
    assert(stm_lcd_panel_subscribe(&panel, complete, &result) == STM_OK);
    assert(stm_lcd_panel_subscribe(&panel, complete, &result) == STM_ERR_INVALID_STATE);
    stm_lcd_panel_handle_t ph = &panel;
    assert(stm_lcd_panel_delete(&ph) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_draw_bitmap_async(&panel, 0, 0, 1, 1, &pixel) == STM_OK);
    hardware_busy = 1;
    assert(stm_lcd_io_complete(&io, STM_OK) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_unsubscribe(&panel, &result) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_delete(&ph) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_io_tx_param(&io, 0, NULL, 0) == STM_ERR_INVALID_STATE);
    hardware_busy = 0;
    assert(stm_lcd_io_complete(&io, STM_OK) == STM_OK && completions == 1 &&
           !stm_lcd_panel_busy(&panel));
    assert(stm_lcd_io_complete(&io, STM_OK) == STM_ERR_INVALID_STATE && completions == 1);
    bad_submit = 1;
    assert(stm_lcd_panel_draw_bitmap_async(&panel, 0, 0, 1, 1, &pixel) == STM_ERR_IO &&
           !stm_lcd_panel_busy(&panel));
    unsafe_submit = 1;
    assert(stm_lcd_panel_draw_bitmap_async(&panel, 0, 0, 1, 1, &pixel) == STM_ERR_IO &&
           stm_lcd_panel_busy(&panel));
    assert(stm_lcd_io_complete(&io, STM_ERR_IO) == STM_ERR_INVALID_STATE);
    hardware_busy = 0;
    unsafe_submit = 0;
    assert(stm_lcd_io_complete(&io, STM_ERR_IO) == STM_OK && completions == 2);
    hardware_busy = 1; // 硬件停止前拒绝再次提交。
    assert(stm_lcd_panel_draw_bitmap_async(&panel, 0, 0, 1, 1, &pixel) == STM_ERR_INVALID_STATE);
    hardware_busy = 0;
    inline_done = 1;
    bad_submit = 0;
    assert(stm_lcd_panel_draw_bitmap_async(&panel, 0, 0, 1, 1, &pixel) == STM_OK &&
           completions == 3 && result == STM_ERR_IO);
    assert(stm_lcd_panel_unsubscribe(&panel, &result) == STM_OK);
    assert(stm_lcd_panel_delete(&ph) == STM_OK && !ph && destroys == 1 && io.ops);
    uint8_t byte;
    assert(stm_lcd_io_set_read_observer(&io, observe, NULL) == STM_OK);
    assert(stm_lcd_io_read(&io, &byte, 1) == STM_OK && observers == 1);
    assert(stm_lcd_io_set_read_observer(&io, NULL, NULL) == STM_OK);
    stm_lcd_touch_config_t config = {.x_max = 170, .y_max = 560, .swap_xy = 1, .mirror_y = 1};
    assert(stm_lcd_touch_base_init(&touch, &touch_ops, &io, &config) == STM_OK);
    assert(stm_lcd_touch_reset(&touch) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_deinit(&io) == STM_ERR_INVALID_STATE);
    stm_lcd_touch_handle_t th = &touch;
    assert(stm_lcd_touch_claim(&touch, &result) == STM_OK);
    assert(stm_lcd_touch_claim(&touch, &result) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_touch_delete(&th) == STM_ERR_INVALID_STATE);
    raw_count = 1;
    raw.x = 0;
    raw.y = 0;
    assert(stm_lcd_touch_read_data(&touch) == STM_OK);
    stm_lcd_touch_point_t point;
    size_t count;
    assert(stm_lcd_touch_get_data(&touch, &point, 1, &count) == STM_OK && count == 1 &&
           point.x == 0 && point.y == 559);
    unsigned before = reads;
    assert(stm_lcd_touch_get_data(&touch, &point, 1, &count) == STM_OK && count == 1 &&
           reads == before);
    raw.x = 559;
    raw.y = 169;
    assert(stm_lcd_touch_read_data(&touch) == STM_OK);
    assert(stm_lcd_touch_get_data(&touch, &point, 1, &count) == STM_OK && count == 1 &&
           point.x == 169 && point.y == 0);
    raw.y = 170;
    assert(stm_lcd_touch_read_data(&touch) == STM_OK);
    assert(stm_lcd_touch_get_data(&touch, &point, 1, &count) == STM_OK && count == 0);
    raw_count = 2;
    assert(stm_lcd_touch_read_data(&touch) == STM_ERR_VERIFY);
    assert(stm_lcd_touch_get_data(&touch, &point, 1, &count) == STM_OK && !count);
    raw_count = 0;
    assert(stm_lcd_touch_read_data(&touch) == STM_OK);
    read_result = STM_ERR_IO;
    assert(stm_lcd_touch_read_data(&touch) == STM_ERR_IO);
    assert(stm_lcd_touch_release(&touch, &pixel) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_touch_release(&touch, &result) == STM_OK);
    assert(stm_lcd_touch_delete(&th) == STM_OK && !th && destroys == 2);
    assert(stm_lcd_io_deinit(&io) == STM_OK && !io.ops);
    puts("Generic IO/panel/touch lifecycle, ownership, callbacks, transforms and snapshot: OK");
    return 0;
}
