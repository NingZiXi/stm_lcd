# stm_lcd：Agent 工作指南

## 范围与阅读顺序

本文件约束本组件目录及子目录；与上级指令合并使用，存在更深层 AGENTS.md 时遵循其局部约束。README 面向使用者，本文件用于 Agent 的接入、修改和验证。

先检查消费工程构建方式、已存在的 target、外设初始化及用户未提交改动，再读本组件 README/CMake/公开头；不覆盖已有业务代码。

## 接入与不可破坏的约束

- 先读 `include/stm_lcd.h` 及 IO/panel/touch 头，再读 `include/stm_lcd_impl.h`。HAL 接入才读 `adapters/stm32f4/README.md` 与适配头。
- 使用者调用通用接口；新驱动在自身对象内嵌入清零 base，不增加分配包装对象或注册中心。
- 仅依赖 `stm_common`，默认不编译 HAL。复用已有 target 或同级源码，缺失明确失败，不为框架新增远程拉取。
- HAL 可选 subdirectory 要求真实 `stm32_hal` target；SPI/I2C target 名为 `stm_lcd_spi_f4`、`stm_lcd_i2c_f4`，DMA 选项为 `STM_LCD_SPI_F4_DMA`。
- IO 属于调用方，设备借用 IO，port 借用设备/缓冲；不能隐式销毁共享外设。ops/context 持续有效。
- 右下矩形边界不包含；可选能力缺失返回 NOT_SUPPORTED，一个面板对应一个 IO 完成订阅者，一个面板只允许一个刷新订阅者。
- 异步 IRQ 只记录邮箱；仅主循环在硬件停止后完成恰好一次。失败但硬件忙时继续保留缓冲，停止失败不能伪造完成。
- 触摸 read 更新单点快照，get 不访问总线且不消费；只在公共框架变换一次：swap → 边界检查 → mirror。
- 测试至少准备同级 `stm_common`。同级两 AXS 驱动存在时额外跑构造分配/回滚集成，不要求独立框架下载芯片驱动。

## 编码与注释

- 中文交流；C11；Allman 括号、4 空格缩进，按本目录 `.clang-format` 格式化，只处理自有代码。
- `.c` / `.h` 文件头仅有 `@file` 和一句 `@brief`，不写 author/date/version/copyright。
- 头文件结构体外的公开函数声明使用 Doxygen：brief、真实参数语义、关键返回值；void 不虚构返回值。
- struct/enum 可在上方写一句说明；成员和枚举项仅在同行右侧用简短 `//`，不使用 `/**< */`，不把字段注释写在上方。
- C 实现不写函数 Doxygen；函数前至多一句 `//`。复杂约束最多 1–3 行，不写教学块、历史或章节分隔线。
- 不改维护者 MIT LICENSE；保留现有第三方来源/许可说明，不把第三方内容直接改署名。

## 验证与交付

从本组件根目录执行，需要主机 C11/C++17 编译器、CMake 3.22+、Ninja：

```sh
cmake -S tests -B build/tests -G Ninja
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

没有同级依赖时先准备源码，不能删测试或伪造空 target 来通过。主机测试不用 MCU 交叉编译器。测试包含 C/C++ 头文件消费；改生命周期/协议时增加对应失败路径测试。

接入消费工程还须使用其工具链编译固件，并运行可用的真实 LVGL / HAL 回归。仅文档注释整理时校验非注释 C token 不变。报告实际依赖版本/提交、修改文件、执行命令、测试/编译结果及未验证项；编译成功不等于实板通过。

未经用户明确授权不烧录、擦写存储、推送、发布版本、修改远程仓库或自动升级依赖。缺少接线、共享复位、目标器件或构建配置时先询问，不猜测。不要修改消费者的 CubeMX 生成文件、HAL 或供应商 LVGL；必要配置列出操作步骤。
