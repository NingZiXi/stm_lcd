# 可选 STM32F4 HAL IO 适配

该目录属于 `stm_lcd`，不属于具体芯片驱动；仅应用选择时编译。应用提供 HAL/CMSIS、外设初始化和 `stm32_hal` target。

- `stm_lcd_spi_f4_init`：调用方静态保存适配对象，使用嵌入的 `io` 句柄。命令和阻塞像素传输由 CS/DC 控制，分块上限 4096 字节。
- DMA：普通模式 byte TX，检查配置的可 DMA SRAM 区间，借用缓冲直到主循环确认 SPI READY、BSY 清除、TXDMAEN 清除、stream 停止。IRQ 只写邮箱。超时/错误执行有界 abort；无法安全停 DMA 时请求复位并保持所有权，不伪造完成。
- 应用显式转发 `HAL_SPI_TxCpltCallback` / `HAL_SPI_ErrorCallback` 到 `stm_lcd_spi_f4_irq_complete/error`，组件不定义全局 HAL 回调。
- 主循环通过 `stm_lcd_io_process(&adapter.io)` 服务；LVGL port 自动调用，不再由 BSP 轮询。底层 poll/tx 函数是适配实现入口，不替代通用接口。
- `stm_lcd_i2c_f4_init`：7-bit 地址，内部左移；独立 Master_Transmit/Receive/IsDeviceReady，保留 STOP，不调用 Mem_Read。config 的 HAL 句柄借用，deinit 不销毁外设。
- `STM_LCD_SPI_F4_DMA=OFF` 可省略 DMA 源。关闭诊断不改变 timeout/stop/ownership 安全机制。
