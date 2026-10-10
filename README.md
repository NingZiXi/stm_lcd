# stm_lcd

[![version 1.0.0](https://img.shields.io/badge/version-1.0.0-5364b5?style=flat-square)](https://github.com/NingZiXi/stm_lcd/releases/tag/v1.0.0)

显示领域的轻量公共框架，将 IO、面板和触摸接口放在**一个组件**中。仅依赖 `stm_common` 的错误定义，不依赖 HAL、LVGL、日志或 RTOS；不提供设备注册中心、后台任务或通用事件队列。

## 🤖 让 Agent 帮助接入

复制以下 Prompt，将 `<...>` 替换为实际需求：

> 将 [stm32-hal-lib](https://github.com/NingZiXi/stm32-hal-lib) 中的 `stm_lcd` 接入当前工程，实现 `<功能>`。硬件：`<MCU/器件型号、外设与引脚、屏幕尺寸与方向（按需填写）>`。先读组件 AGENTS.md 和 README，核对配置，保留已有代码；缺失信息先询问，不猜接线。完成后说明版本、编译结果和未验证项；未经确认不烧录或擦写。

💡 可使用 [Gitee 镜像](https://gitee.com/nzxhg/stm32-hal-lib)。需要业务入口和日志时，参考 [skills/README.md](https://github.com/NingZiXi/stm32-hal-lib/blob/main/skills/README.md)（仅适用于已有 CubeMX1 CMake 工程）；编译通过不代表实板验证通过。

## 组件关系与目录

```text
应用：板级配置、复位协调、静态缓冲、UI
  └─ stm_lvgl_port：LVGL 刷新、输入及协作式运行
      ├─ stm_lcd_axs15231b：具体面板协议
      └─ stm_lcd_touch_axs15231b：具体触摸协议
          └─ stm_lcd：IO / panel / touch 公共契约
              └─ 可选 adapters/stm32f4：HAL SPI/DMA 与原始 I2C
```

- `include/stm_lcd.h`：使用者总入口；也可分别包含 IO、panel、touch 头文件。
- `include/stm_lcd_impl.h`：驱动实现者操作表与 base，不供业务直接修改内部状态。
- `src/`：参数校验、所有权、完成分发和坐标快照。
- `adapters/stm32f4/`：可选 HAL 适配及[配置说明](adapters/stm32f4/README.md)。
- `tests/`：假 IO、可替换设备、生命周期及 C/C++ 头文件回归。

## CMake 接入

先复用 `stm_common` target，否则使用同级源码；缺失明确报错，框架自身不下载依赖。

```cmake
# 已存在 stm_common target 时不要重复添加。
add_subdirectory(Lib/stm_common)
add_subdirectory(Lib/stm_lcd)
target_link_libraries(your_firmware PRIVATE stm_lcd)
```

默认不编译 HAL。STM32F4 应用先提供 `stm32_hal` target，包含真实 HAL/CMSIS include、MCU 宏及必要链接依赖，再选择：

```cmake
add_subdirectory(Lib/stm_lcd/adapters/stm32f4)
target_link_libraries(your_firmware PRIVATE stm_lcd_spi_f4 stm_lcd_i2c_f4)
# 纯同步可在 add_subdirectory 之前 set(STM_LCD_SPI_F4_DMA OFF CACHE BOOL "")。
```

不要用空的 `stm32_hal` target 掩盖缺失 HAL 配置。其他 MCU 可自行实现 IO 操作表，无需修改面板或 port。

## 使用者接入顺序

1. 应用初始化时钟、GPIO、SPI/I2C、DMA 和 IRQ，保存清零的 IO 适配对象。
2. 初始化 IO，调用具体驱动构造函数得到通用面板/触摸句柄。
3. 按模组要求协调复位、初始化面板并开启显示；共享复位不能在接入后随意操作。
4. 直接使用通用接口，或将设备和静态缓冲交给 `stm_lvgl_port`。
5. 主循环持续服务 `stm_lcd_io_process()`；使用 port 时由其自动服务，不重复轮询。
6. 停止新提交并服务所有在途请求；先删除 port，再删除设备，最后 deinit IO。

具体可运行接入片段见两个驱动的 README 和示例；该框架不猜测引脚，也不创建芯片对象。

## 公共契约

| 模块 | 常用操作 | 关键语义 |
| --- | --- | --- |
| IO | `tx_param`、`tx_color`、`tx_color_async` | 同步返回后归还缓冲；异步安全完成前持续借用 |
| IO | `write`、`read`、`probe`、`process` | 独立原始 I2C 事务；探测不复位；process 在主循环运行 |
| panel | `reset`、`init`、`display_on_off`、`draw_bitmap` | 绘图为 RGB565，矩形右下边界不包含 |
| panel | `subscribe`、`unsubscribe`、`delete` | 一个刷新所有者；在途或回调中不能退订/删除 |
| touch | `reset`、`read_data`、`get_data` | read 更新快照；get 不访问总线且不消费快照 |
| touch | `claim`、`release`、`delete` | 输入所有者借用期间不能复位/删除 |

- 调用方持有 IO/硬件；面板与触摸借用 IO；port 借用设备和缓冲。删除设备不销毁共享资源。
- 一个 IO 的像素完成只被一个面板占用；多个面板应分别使用独立 IO 对象，不是多屏管理器。
- 触摸当前保存最多一个触点，变换顺序为交换轴 → 逻辑边界检查 → 镜像；越界、零触点、读取错误均清空快照。
- 所有通用 API 串行运行于主循环，禁止 ISR 调用、递归访问、在观察回调中删除对象。HAL IRQ 只调用适配层邮箱入口。

## 新驱动或 IO 的实现约束

公共 base 嵌入实现对象，初始化前清零；ops、context 必须比对象存活更久。由具体构造函数分配自己的小对象，不额外分配包装层；构造失败保持空输出，非空输出不得覆盖。

异步提交前框架标记 pending。实现者仅在主循环、DMA 与总线均停止访问缓冲后调用 `stm_lcd_io_complete()`，每个请求恰好一次。提交失败但硬件仍忙时不能释放缓冲；必须继续服务超时/终止。重复、异常或硬件仍忙的完成被拒绝。不要在 ISR 中调用公共 complete。

触摸操作表返回原始单点，坐标变换只由公共框架处理；不要在芯片驱动和应用各变换一次。

## 返回值与排查

| 返回值 | 含义与处理 |
| --- | --- |
| `STM_ERR_INVALID_ARG` / `STM_ERR_INVALID_CONFIG` | 检查空句柄、操作表、尺寸及布尔配置 |
| `STM_ERR_INVALID_STATE` | 对象忙、借用冲突或非法生命周期；先服务在途操作，不强制清状态 |
| `STM_ERR_NOT_SUPPORTED` | 可选操作未实现；先查询能力或按场景处理 |
| `STM_ERR_OUT_OF_RANGE` | 检查矩形及传输容量 |
| `STM_ERR_IO` / `STM_ERR_TIMEOUT` | 保留错误并继续完成服务，确认硬件停止后才回收缓冲 |
| `STM_ERR_VERIFY` | 触摸报文无效；不要将无效快照当作按下 |

## 主机测试

需要 CMake 3.22+、Ninja、支持 C11/C++17 的主机编译器。从**组件根目录**执行：

```sh
cmake -S tests -B build/tests -G Ninja
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

测试时需将 `stm_common` 和 `stm_lcd` 放在本组件同级（`stm_lcd` 自身只需同级 `stm_common`），或按测试 CMake 的要求准备依赖。嵌入式交叉编译器不用于运行主机测试。可显式传入 `-DCMAKE_C_COMPILER=<主机C编译器>` 和 `-DCMAKE_CXX_COMPILER=<主机C++编译器>`。

覆盖可选能力、所有权、立即/重复/提前完成、失败提交保护、坐标端点及非消费快照。同级两个 AXS 驱动存在时额外覆盖分配失败与构造回滚；独立框架测试不要求下载芯片驱动。

## 验证范围与许可

2026-10-10，消费工程在 STM32F407 + AXS15231B、21 MHz SPI、170×560 原生竖屏、RGB565、两个 16 行普通 SRAM 缓冲的配置下完成显示和触摸验收。当前文档整理不改变这一运行配置。

该结果不等于其他模组、其他 MCU 或长时间稳定性验收；既有触摸畸形帧问题不能标记为已修复。当前正式版本为 `v1.0.0`，统一接口不兼容重构前 API；接入时以该版本公开头文件为准，不将旧版本的接口或验收结论套用到本版本。

维护者的 MIT 许可证保持原内容，见 [LICENSE](LICENSE)。源码中已有的第三方来源及许可说明保持保留。Agent 工作约束见 [AGENTS.md](AGENTS.md)。
