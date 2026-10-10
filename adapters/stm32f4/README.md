# 可选 STM32F4 HAL IO 适配

该目录属于 `stm_lcd`，不属于芯片驱动。仅应用选择时编译；应用提供真实 HAL/CMSIS、MCU 宏、外设初始化及 `stm32_hal` target。适配不占用全局 HAL 回调，不猜测引脚，不操作板级背光或 PD15。

## 构建与对象

```cmake
# stm_lcd 和真实 stm32_hal target 已由应用提供。
# 纯同步时可在此之前 set(STM_LCD_SPI_F4_DMA OFF CACHE BOOL "")。
add_subdirectory(Lib/stm_lcd/adapters/stm32f4)
target_link_libraries(your_firmware PRIVATE stm_lcd_spi_f4 stm_lcd_i2c_f4)
```

调用方静态保存清零的 `stm_lcd_spi_f4_t` / `stm_lcd_i2c_f4_t`。init 只复制配置并建立 IO 契约，不执行 HAL 外设初始化。通用句柄为 `&adapter.io`；业务通过 `stm_lcd_io_*` 使用，底层 tx/poll 为适配实现入口。

## SPI / DMA 必备配置

- SPI 与 CS/DC 必须已初始化，CS 低有效；一个适配独占一个 SPI 外设，不支持并发共享总线仲裁。
- `timeout_ms` 非零且不超过 INT32_MAX；同步传输按最多 4096 字节分块。
- `enable_dma=1` 需编译 DMA 源；HAL TX DMA 使用 normal mode、byte 数据宽度，正确关联 hdmatx。
- 正确启用专用 SPI/DMA IRQ；dma_irq/spi_irq 非负且不同，硬件初始化和 HAL IRQ handler 由应用提供。
- `dma_memory_begin/end` 是可 DMA 访问的连续 SRAM 半开区间，按 MCU/linker 实际布局填写；缓冲完整位于该区间且单次字节数不超过 UINT16_MAX。STM32F407 的 CCM 不能用于此缓冲。
- 关闭诊断展示不能关闭 timeout、终止、错误锁存和所有权保护。

## 显式回调转发

将以下转发**合并到应用已有回调**，保留其他 SPI 的分支，不创建第二份同名 HAL 回调。`lcd_spi` 是应用持有的适配对象：

```c
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    stm_lcd_spi_f4_irq_complete(&lcd_spi, hspi);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    stm_lcd_spi_f4_irq_error(&lcd_spi, hspi);
}
```

适配忽略非目标 SPI。IRQ 只记录邮箱，不调用 LVGL、通用 complete、I2C、日志或切换 CS。主循环调用 `stm_lcd_io_process(&lcd_spi.io)`；使用 port 时由 port 自动调用，不由 BSP 重复轮询。

完成归还像素前，主循环确认 SPI READY、BSY 清除、TXDMAEN 清除、DMA stream 停止。超时/错误执行有界终止；硬件无法停止时请求系统复位并保留所有权，不能伪造完成。失败提交后若通用 IO 仍 busy，必须继续服务而不是释放/复用缓冲。

## 原始 I2C

`stm_lcd_i2c_f4_config_t.address_7bit` 使用未左移的 7 位地址，适配内部左移。timeout 非零且不超过 INT32_MAX。使用独立 Master_Transmit、Master_Receive、IsDeviceReady，写读之间保留 STOP，不调用 Mem_Read。

AXS15231B 触摸使用 0x3B，构造后传给 port 可由其统一采样和掉线探测；不要另跑板级输入状态机。共享复位由应用在接入前协调，不在探测重试时自动复位。

## 释放与验证

先停止新提交并服务所有在途请求，删除 port → 删除 panel/touch → 适配 deinit。deinit 不销毁借用 HAL 外设；仍有借用者或传输时拒绝清除。

[框架 README](../../README.md) 的主机测试验证公共契约；实际 DMA 停止、IRQ 和 SRAM 地址还需消费工程 HAL 回归及实板验证。编译通过不等于硬件验收，未经确认不烧录或擦写。
