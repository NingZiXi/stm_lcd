/** @file stm_lcd_impl.h @brief Driver author API; embed bases, do not mutate live state. */
#ifndef STM_LCD_IMPL_H
#define STM_LCD_IMPL_H
#include "stm_lcd.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct
{
    stm_err_t (*tx_param)(void *,
                          uint8_t,
                          const uint8_t *,
                          size_t); /**< Optional command transaction. */
    stm_err_t (*tx_color)(void *,
                          uint8_t,
                          const void *,
                          size_t); /**< Optional blocking pixel transaction. */
    stm_err_t (*tx_color_async)(void *,
                                uint8_t,
                                const void *,
                                size_t); /**< Failure must stop accessing pixels. */
    stm_err_t (*write)(void *,
                       const uint8_t *,
                       size_t); /**< Raw independent I2C transaction, with STOP. */
    stm_err_t (*read)(void *, uint8_t *, size_t); /**< Raw independent I2C transaction. */
    stm_err_t (*probe)(void *);                   /**< Optional connectivity check, no reset. */
    void (*process)(void *);                      /**< Main-loop completion/timeout service. */
    int (*busy)(void *); /**< Hardware access safety fallback, zero when stopped. */
} stm_lcd_io_ops_t;

struct stm_lcd_io
{
    const stm_lcd_io_ops_t *ops;            /**< Implementation must outlive the IO. */
    void *context;                          /**< Borrowed implementation context. */
    stm_lcd_complete_cb complete;           /**< Sole device subscriber. */
    void *owner;                            /**< Subscriber identity. */
    stm_lcd_read_observer_cb read_observer; /**< Diagnostics only. */
    void *read_context;                     /**< Borrowed diagnostics context. */
    uint16_t borrowers;                     /**< Live panel/touch references, blocks deinit. */
    uint8_t pending, dispatching;           /**< Ownership and callback guards, main-loop only. */
};

typedef struct
{
    stm_err_t (*reset)(stm_lcd_panel_handle_t);               /**< Optional hardware reset. */
    stm_err_t (*init)(stm_lcd_panel_handle_t);                /**< Controller initialization. */
    stm_err_t (*display_on_off)(stm_lcd_panel_handle_t, int); /**< Optional display switch. */
    stm_err_t (*set_window)(stm_lcd_panel_handle_t,
                            uint16_t,
                            uint16_t,
                            uint16_t,
                            uint16_t); /**< Optional window API. */
    stm_err_t (*draw)(stm_lcd_panel_handle_t,
                      uint16_t,
                      uint16_t,
                      uint16_t,
                      uint16_t,
                      const void *); /**< Blocking draw. */
    stm_err_t (*draw_async)(stm_lcd_panel_handle_t,
                            uint16_t,
                            uint16_t,
                            uint16_t,
                            uint16_t,
                            const void *);   /**< Borrow until completion. */
    void (*destroy)(stm_lcd_panel_handle_t); /**< Free implementation only, not borrowed IO. */
} stm_lcd_panel_ops_t;

struct stm_lcd_panel
{
    const stm_lcd_panel_ops_t *ops; /**< Static implementation operations. */
    stm_lcd_io_handle_t io;         /**< Borrowed IO. */
    uint16_t width, height;         /**< Logical bounds. */
    stm_lcd_complete_cb complete;   /**< Exclusive refresh subscriber. */
    void *owner;                    /**< Subscriber identity. */
    uint8_t pending, dispatching;   /**< Completion and destruction guards. */
};

typedef struct
{
    stm_err_t (*reset)(stm_lcd_touch_handle_t); /**< Optional startup reset. */
    stm_err_t (*read_data)(stm_lcd_touch_handle_t,
                           stm_lcd_touch_point_t *,
                           size_t *);        /**< Decode one raw point, no transform. */
    void (*destroy)(stm_lcd_touch_handle_t); /**< Free implementation only. */
} stm_lcd_touch_ops_t;

struct stm_lcd_touch
{
    const stm_lcd_touch_ops_t *ops; /**< Static implementation operations. */
    stm_lcd_io_handle_t io;         /**< Borrowed IO. */
    stm_lcd_touch_config_t config;  /**< Generic coordinate transform. */
    stm_lcd_touch_point_t point;    /**< Logical non-consuming snapshot. */
    void *owner;                    /**< Optional input owner, prevents deletion while borrowed. */
    uint8_t count, accessing;       /**< Snapshot validity and operation guard. */
};

/* Init requires zeroed bases. Completion is main-loop only, exactly once after
 * wire and DMA stopped. Duplicate/unexpected completions return INVALID_STATE. */
stm_err_t stm_lcd_io_init(stm_lcd_io_handle_t io, const stm_lcd_io_ops_t *ops, void *context);
stm_err_t stm_lcd_io_complete(stm_lcd_io_handle_t io, stm_err_t result);
stm_err_t stm_lcd_panel_base_init(stm_lcd_panel_handle_t panel,
                                  const stm_lcd_panel_ops_t *ops,
                                  stm_lcd_io_handle_t io,
                                  uint16_t width,
                                  uint16_t height);
stm_err_t stm_lcd_touch_base_init(stm_lcd_touch_handle_t touch,
                                  const stm_lcd_touch_ops_t *ops,
                                  stm_lcd_io_handle_t io,
                                  const stm_lcd_touch_config_t *config);
#ifdef __cplusplus
}
#endif
#endif
