#include "stm_lcd_impl.h"
#include "stm_lcd_axs15231b.h"
#include "stm_lcd_touch_axs15231b.h"
#include <assert.h>
#include <stdlib.h>

static unsigned live, allocations, releases;
static int fail_allocation;

void *allocation_calloc(size_t count, size_t size)
{
    ++allocations;
    if (fail_allocation)
        return NULL;
    void *p = calloc(count, size);
    if (p)
        ++live;
    return p;
}

void allocation_free(void *p)
{
    if (p)
    {
        assert(live);
        --live;
        ++releases;
    }
    free(p);
}

static stm_err_t param(void *c, uint8_t cmd, const uint8_t *data, size_t size)
{
    (void)c;
    (void)cmd;
    (void)data;
    (void)size;
    assert(0 && "constructors must not access hardware");
    return STM_OK;
}

static stm_err_t color(void *c, uint8_t cmd, const void *data, size_t size)
{
    return param(c, cmd, data, size);
}

static stm_err_t write_data(void *c, const uint8_t *data, size_t size)
{
    return param(c, 0, data, size);
}

static stm_err_t read_data(void *c, uint8_t *data, size_t size)
{
    return param(c, 0, data, size);
}

static void delay(void *c, uint32_t ms)
{
    (void)c;
    (void)ms;
    assert(0 && "constructors must not delay");
}

int main(void)
{
    const stm_lcd_io_ops_t ops = {
        .tx_param = param, .tx_color = color, .write = write_data, .read = read_data};
    struct stm_lcd_io io = {0};
    assert(stm_lcd_io_init(&io, &ops, NULL) == STM_OK);
    stm_lcd_panel_handle_t panel = NULL, other = NULL;
    stm_lcd_touch_handle_t touch = NULL;
    lcd_axs15231b_config_t pc = {.io = &io, .width = 170, .height = 560, .delay_ms = delay};
    lcd_touch_axs15231b_config_t tc = {
        .io = &io, .coordinates = {.x_max = 170, .y_max = 560, .swap_xy = 1, .mirror_y = 1}};

    fail_allocation = 1;
    assert(lcd_axs15231b_create(&pc, &panel) == STM_ERR_NO_MEM);
    assert(!panel && !io.borrowers && !io.complete && !live);
    assert(lcd_touch_axs15231b_create(&tc, &touch) == STM_ERR_NO_MEM);
    assert(!touch && !io.borrowers && !live);
    fail_allocation = 0;

    assert(lcd_axs15231b_create(&pc, &panel) == STM_OK);
    assert(panel && io.borrowers == 1 && live == 1);
    unsigned freed = releases;
    assert(lcd_axs15231b_create(&pc, &other) == STM_ERR_INVALID_STATE);
    assert(!other && live == 1 && releases == freed + 1 && io.borrowers == 1);
    /* An existing output is preserved, never leaked or silently replaced. */
    stm_lcd_panel_handle_t saved = panel;
    assert(lcd_axs15231b_create(&pc, &panel) == STM_ERR_INVALID_STATE);
    assert(panel == saved && live == 1);
    assert(stm_lcd_panel_delete(&panel) == STM_OK);
    assert(!panel && !live && !io.borrowers && !io.complete);

    tc.coordinates.x_max = 0; /* Failure after allocation must release the object. */
    freed = releases;
    assert(lcd_touch_axs15231b_create(&tc, &touch) == STM_ERR_INVALID_CONFIG);
    assert(!touch && !live && releases == freed + 1 && !io.borrowers);
    tc.coordinates.x_max = 170;
    assert(lcd_touch_axs15231b_create(&tc, &touch) == STM_OK);
    assert(touch && live == 1 && io.borrowers == 1);
    assert(stm_lcd_touch_delete(&touch) == STM_OK);
    assert(!touch && !live && !io.borrowers);
    assert(stm_lcd_io_deinit(&io) == STM_OK);
    assert(allocations == 6 && releases == 4);
    return 0;
}
