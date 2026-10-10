/**
 * @file test_framebuffer.c
 * @brief 验证整帧所有权、停止失败及原子寄存器事务。
 */
#include "stm_lcd_impl.h"
#include <assert.h>
#include <string.h>

typedef struct
{
    struct stm_lcd_io io;                                         // 通用 IO 与借用状态。
    struct stm_lcd_panel panel;                                   // 模型面板。
    unsigned submits, stops, completed, reads, observed, regions; // 提交、停止与完成计数。
    int busy;                                                     // 模拟在途状态。
    stm_err_t submit_error, stop_error, result, register_error;   // 注入错误与最终结果。
    const void *active, *next; // 当前扫描帧与待切换帧。
} mock_t;

static int busy(void *ctx)
{
    return ((mock_t *)ctx)->busy;
}

static stm_err_t present(void *ctx, const void *pixels, uint16_t w, uint16_t h)
{
    mock_t *m = ctx;
    assert(w == 2 && h == 2 && pixels != m->active);
    ++m->submits;
    m->next = pixels;
    return m->submit_error;
}

static stm_err_t stop(void *ctx)
{
    mock_t *m = ctx;
    ++m->stops;
    if (m->stop_error != STM_OK)
    {
        return m->stop_error;
    }
    m->active = m->next = NULL;
    return STM_OK;
}

static void process(void *ctx)
{
    (void)ctx;
}

static stm_err_t
draw_region(void *ctx, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const void *pixels)
{
    mock_t *m = ctx;
    assert(x1 == 0 && y1 == 0 && x2 == 1 && y2 == 1 && pixels);
    ++m->regions;
    return m->register_error;
}

static stm_err_t read_reg(void *ctx, uint16_t reg, uint8_t width, uint8_t *data, size_t n)
{
    mock_t *m = ctx;
    assert(reg == 0x8140 && width == 2 && n == 4);
    ++m->reads;
    if (m->register_error != STM_OK)
    {
        return m->register_error;
    }
    memcpy(data, "9271", 4);
    return STM_OK;
}

static stm_err_t write_reg(void *ctx, uint16_t reg, uint8_t width, const uint8_t *data, size_t n)
{
    mock_t *m = ctx;
    assert(reg == 0x814e && width == 2 && n == 1 && *data == 0);
    return m->register_error;
}

static void observe(void *ctx, const uint8_t *data, size_t n)
{
    mock_t *m = ctx;
    assert(n == 4 && !memcmp(data, "9271", 4));
    ++m->observed;
    assert(stm_lcd_io_deinit(&m->io) == STM_ERR_INVALID_STATE);
}

static void complete(void *ctx, stm_err_t result)
{
    mock_t *m = ctx;
    ++m->completed;
    m->result = result;
}

static stm_err_t panel_present(stm_lcd_panel_handle_t p, const void *pixels)
{
    return stm_lcd_io_present(p->io, pixels, p->width, p->height);
}

static void destroy(stm_lcd_panel_handle_t p)
{
    memset(p, 0, sizeof(*p));
}

static const stm_lcd_io_ops_t ops = {.present = present,
                                     .stop_scanout = stop,
                                     .busy = busy,
                                     .process = process,
                                     .read_reg = read_reg,
                                     .write_reg = write_reg,
                                     .draw_region = draw_region};
static const stm_lcd_io_ops_t empty = {0};
static const stm_lcd_panel_ops_t panel_ops = {.present = panel_present, .destroy = destroy};

int main(void)
{
    mock_t m = {0};
    struct stm_lcd_io no = {0};
    uint16_t a[4] = {0}, b[4] = {0};
    uint8_t id[4], zero = 0;
    assert(stm_lcd_io_init(&m.io, &ops, &m) == STM_OK);
    assert(stm_lcd_io_init(&no, &empty, NULL) == STM_OK);
    assert(!stm_lcd_io_supports_present(&no));
    assert(stm_lcd_io_present(&no, a, 2, 2) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_present(&m.io, NULL, 2, 2) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_io_present(&m.io, a, 0, 2) == STM_ERR_INVALID_ARG && m.submits == 0);
    assert(stm_lcd_io_draw_region(&m.io, 0, 0, 0, 1, a) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_io_draw_region(&m.io, 0, 0, 1, 1, a) == STM_OK && m.regions == 1);
    assert(stm_lcd_io_read_reg(&m.io, 0x8140, 1, id, 4) == STM_ERR_INVALID_ARG && !m.reads);
    assert(stm_lcd_io_read_reg(&m.io, 0, 3, id, 4) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_io_read_reg(&no, 0, 1, id, 4) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_write_reg(&no, 0, 1, &zero, 1) == STM_ERR_NOT_SUPPORTED);
    assert(stm_lcd_io_set_read_observer(&m.io, observe, &m) == STM_OK);
    assert(stm_lcd_io_read_reg(&m.io, 0x8140, 2, id, 4) == STM_OK && m.observed == 1);
    m.register_error = STM_ERR_TIMEOUT;
    assert(stm_lcd_io_read_reg(&m.io, 0x8140, 2, id, 4) == STM_ERR_TIMEOUT && m.observed == 1);
    assert(stm_lcd_io_write_reg(&m.io, 0x814e, 2, &zero, 1) == STM_ERR_TIMEOUT);
    assert(stm_lcd_panel_base_init(&m.panel, &panel_ops, &m.io, 2, 2) == STM_OK);
    assert(stm_lcd_panel_subscribe(&m.panel, complete, &m) == STM_OK);
    assert(stm_lcd_panel_present(&m.panel, a) == STM_OK && m.io.scanout);
    assert(stm_lcd_panel_present(&m.panel, b) == STM_ERR_INVALID_STATE);
    m.busy = 1;
    assert(stm_lcd_io_complete(&m.io, STM_OK) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_stop_scanout(&m.panel) == STM_ERR_INVALID_STATE);
    m.busy = 0;
    m.active = m.next;
    assert(stm_lcd_io_complete(&m.io, STM_OK) == STM_OK);
    assert(m.completed == 1 && m.result == STM_OK && m.io.scanout && !stm_lcd_panel_busy(&m.panel));
    assert(stm_lcd_io_complete(&m.io, STM_OK) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_reset(&m.panel) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_init(&m.panel) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_present(&m.panel, b) == STM_OK);
    m.stop_error = STM_ERR_IO;
    assert(stm_lcd_io_complete(&m.io, STM_ERR_TIMEOUT) == STM_ERR_IO && m.completed == 1 &&
           stm_lcd_panel_busy(&m.panel));
    m.stop_error = STM_OK;
    assert(stm_lcd_io_complete(&m.io, STM_OK) == STM_OK && m.result == STM_ERR_TIMEOUT &&
           !m.io.scanout);
    m.submit_error = STM_ERR_CANCELLED;
    assert(stm_lcd_panel_present(&m.panel, a) == STM_ERR_CANCELLED &&
           !stm_lcd_panel_busy(&m.panel) && !m.io.scanout);
    m.stop_error = STM_ERR_IO;
    assert(stm_lcd_panel_present(&m.panel, a) == STM_ERR_CANCELLED && stm_lcd_panel_busy(&m.panel));
    m.stop_error = STM_OK;
    assert(stm_lcd_io_complete(&m.io, STM_OK) == STM_OK && m.result == STM_ERR_CANCELLED);
    m.submit_error = STM_OK;
    assert(stm_lcd_panel_present(&m.panel, a) == STM_OK);
    m.active = m.next;
    assert(stm_lcd_io_complete(&m.io, STM_OK) == STM_OK);
    assert(stm_lcd_panel_unsubscribe(&m.panel, &m) == STM_OK);
    stm_lcd_panel_handle_t panel = &m.panel;
    assert(stm_lcd_panel_delete(&panel) == STM_ERR_INVALID_STATE);
    m.stop_error = STM_ERR_IO;
    assert(stm_lcd_panel_stop_scanout(panel) == STM_ERR_IO && panel);
    m.stop_error = STM_OK;
    assert(stm_lcd_panel_stop_scanout(panel) == STM_OK);
    assert(stm_lcd_panel_delete(&panel) == STM_OK && !panel);
    assert(stm_lcd_io_deinit(&m.io) == STM_OK && stm_lcd_io_deinit(&no) == STM_OK);
    return 0;
}
