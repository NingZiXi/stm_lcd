# stm_lcd

显示领域公共契约：IO、panel、touch 共用一个组件。仅依赖 `stm_common` 的错误定义，不依赖 HAL、LVGL、日志、RTOS。不是跨领域设备管理器，也没有任务/队列/注册中心。

## 接入与实现

- 头文件：`stm_lcd.h`（也可使用三个模块头）；实现者使用 `stm_lcd_impl.h`，把公共 base 嵌入驱动对象，不分配额外包装对象。
- 调用方持有 IO/硬件，面板/触摸借用 IO，port 借用设备和缓冲。删除设备不销毁 IO；IO 有借用者时拒绝 deinit。
- 右下绘图边界不包含。未实现的可选操作返回 `STM_ERR_NOT_SUPPORTED`。
- 异步 IO 在提交前标记 pending。完成必须在主循环、DMA/SPI 已停止访问后调用 `stm_lcd_io_complete`；提前、重复完成拒绝。提交错误且硬件仍 busy 时继续持有缓冲。
- 一个 IO 的像素完成由一个面板占用；一个面板只有一个刷新订阅者。pending/回调期间禁止删除或退订。不同面板应使用独立 IO 对象。
- touch `read_data` 更新一个快照；`get_data` 不访问总线也不消费快照。变换顺序统一为 swap → 逻辑边界检查 → mirror，越界为空快照。错误和零触点清除快照。
- 所有 API 只在串行主循环使用，禁止 ISR 调用、观察回调中的递归访问/删除。ops、context 必须持续有效，base 初始化前清零。

## 构建

`add_subdirectory(Lib/stm_lcd)`。先复用 `stm_common` target，否则使用同级源码；缺失明确报错，不下载远程版本。
HAL 可选：提供 `stm32_hal` 接口 target，再 `add_subdirectory(Lib/stm_lcd/adapters/stm32f4)`，链接 `stm_lcd_spi_f4` / `stm_lcd_i2c_f4`。默认框架不编译 HAL。

## 测试

`cmake -S Lib/stm_lcd/tests -B build/lcd-tests -G Ninja`，构建后运行 CTest。包括 C11/C++17 头文件、可选方法、所有权、inline/重复/提前完成、失败提交安全、变换端点和非消费快照；同级两个 AXS 驱动存在时，另一个集成用例覆盖芯片构造分配失败及中途失败回滚；独立克隆框架不要求下载芯片驱动。

## 验证与许可

2026-10-10，在 STM32F407 + AXS15231B 消费工程以 21 MHz SPI、170×560 原生竖屏、RGB565、两个 16 行普通 SRAM 缓冲运行，用户确认显示与触摸正常。该结论仅覆盖上述配置，未作为独立长时间 soak 验收，也不宣称既有触摸畸形帧问题已修复。当前为未发布开发提交，未创建新 tag 或 Release。

自有源码沿用维护者原 MIT，见 [LICENSE](LICENSE)。
