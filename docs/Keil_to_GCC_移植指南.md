# GD32F303CCT6 Keil → GCC 移植踩坑全记录

> **项目**: GD32F303CCT6 PD/PPS 快充测试固件  
> **MCU**: GD32F303CCT6 (Cortex-M4F, 256KB Flash, 48KB RAM)  
> **源工具链**: Keil MDK (ARMCC v5)  
> **目标工具链**: ARM GNU Toolchain 15.3 + CMake + Ninja + OpenOCD  
> **移植日期**: 2026-08-01  
> **结果**: ✅ 编译、烧录、运行全部通过

---

## 目录

1. [环境准备](#1-环境准备)
2. [需要新建的文件](#2-需要新建的文件)
3. [编译器差异导致的语法问题](#3-编译器差异导致的语法问题)
4. [链接器踩坑](#4-链接器踩坑)
5. [OpenOCD 烧录与调试配置](#5-openocd-烧录与调试配置)
6. [运行时崩溃问题的根因分析与修复](#6-运行时崩溃问题的根因分析与修复)
7. [最终的项目文件结构](#7-最终的项目文件结构)
8. [踩坑速查表](#8-踩坑速查表)

---

## 1. 环境准备

### 1.1 安装 ARM GNU Toolchain

从 ARM 官网下载 Windows 版工具链，安装到 `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\`。

验证安装:
```bash
arm-none-eabi-gcc --version
# arm-none-eabi-gcc (Arm GNU Toolchain 15.3 ...) 15.3.1
```

### 1.2 安装 OpenOCD（可选，用于烧录/调试）

下载 [xpack-openocd](https://github.com/xpack-dev-tools/openocd-xpack) 或 sysprogs 版本。配置好 stlink 驱动（下载 STSW-LINK009 或安装 Zadig）。

**注意**: 建议使用 openocd 官方版本 (`openocd.org` 构建)，因为 sysprogs fork (0.12.0) 在语法上有些差异（详见第 5 节），直接用 `-f interface/stlink.cfg -f target/stm32f3x.cfg` 最省事。

### 1.3 VSCode 扩展

- **C/C++** (Microsoft)
- **CMake Tools** (Microsoft)
- **Cortex-Debug** (marus25) — GDB 调试必备

---

## 2. 需要新建的文件

Keil 项目使用 `.uvprojx` + armasm 启动文件 + scatter 分散加载文件，GCC 需要完全不同的基础设施：

| 文件 | 用途 | 替代 Keil 中的 |
|------|------|----------------|
| `CMakeLists.txt` | 构建系统 | `.uvprojx` |
| `cmake/arm-gcc-toolchain.cmake` | 交叉编译工具链 | Keil 内置 |
| `CMSIS/startup_gd32f30x_hd_gcc.s` | GCC 启动文件 | `startup_gd32f30x_hd.s` (armasm) |
| `GD32F303CC.ld` | 链接脚本 | `.sct` 分散加载文件 |
| `User/syscalls.c` | newlib 系统调用桩 | Keil microLib 自动提供 |
| `.vscode/launch.json` | 调试配置 | Keil Debug |
| `.vscode/tasks.json` | 构建/烧录任务 | Keil 内置 |

### 2.1 CMakeLists.txt 关键设计

```cmake
cmake_minimum_required(VERSION 3.20)
project(GD32F303CCT6_Pro LANGUAGES C ASM)

# 功能模块通过 option 开关控制
option(CONFIG_TCPC_FUNC "Enable USB PD" OFF)
option(CONFIG_SCP_FUNC  "Enable SCP"   OFF)

# 添加交叉编译工具链: -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
```

五层静态库结构（`cmsis` → `fwlib` → `hardware` → `cmbacktrace` → 最终 elf），依赖链清晰，按需链接。

### 2.2 arm-gcc-toolchain.cmake 关键点

```cmake
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# ⚠️ 必须用 find_program 定位完整路径
# 否则 Ninja 在 POST_BUILD 阶段找不到 arm-none-eabi-size/objcopy
find_program(CMAKE_C_COMPILER   arm-none-eabi-gcc    PATHS ${CMAKE_PROGRAM_PATH})
find_program(CMAKE_OBJCOPY      arm-none-eabi-objcopy PATHS ${CMAKE_PROGRAM_PATH})
find_program(CMAKE_SIZE         arm-none-eabi-size    PATHS ${CMAKE_PROGRAM_PATH})

# Cortex-M4F 配置
set(CPU_FLAGS
  -mcpu=cortex-m4 -mthumb
  -mfloat-abi=hard -mfpu=fpv4-sp-d16     # HW FPU!
)

# ⚠️ CMake list → string: 必须用 string(JOIN)，不能直接拼到 flag 变量
string(JOIN " " CPU_FLAGS_STR ${CPU_FLAGS})
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS_STR} -std=c99 -Wall ...")
```

> **踩坑**: CMake 的 flag 变量必须是字符串，而 `set(CPU_FLAGS ...)` 创建的是列表（分号分隔）。直接用 `${CPU_FLAGS}` 拼接到 `CMAKE_C_FLAGS_INIT` 会导致分号被传入，GCC 不识别。

### 2.3 启动文件 — armasm → GCC asm 转换要点

| Keil armasm | GCC asm | 说明 |
|-------------|---------|------|
| `EXPORT Reset_Handler` | `.globl Reset_Handler` | 导出符号 |
| `IMPORT SystemInit` | 无需声明，链接时解析 | 导入符号 |
| `PROC / ENDP` | `.type name, %function` + `bx lr` | 函数定义 |
| `DCD _estack` | `.word _estack` | 数据常量 |
| `AREA RESET, DATA, READONLY` | `.section .isr_vector, "a"` | 段定义 |
| `SPACE 0x400` | 用 `.bss` 段 + linker script 管理 | 堆栈空间 |

**关键代码段 — 正确的 Reset_Handler:**

```asm
Reset_Handler:
  /* ===== VTOR 初始化 (必须放在最前面!) ===== */
  ldr   r0, =0xE000ED08
  ldr   r1, =0x08000000
  str   r1, [r0]
  dsb
  isb

  /* Copy .data from Flash to RAM */
  ldr   r0, =__data_load__
  ldr   r1, =__data_start__
  ldr   r2, =__data_end__
  subs  r2, r1
  beq   .L_zero_bss
.L_copy_data:
  ldrb  r3, [r0], #1
  strb  r3, [r1], #1
  subs  r2, #1
  bne   .L_copy_data

.L_zero_bss:
  /* Zero .bss */
  ldr   r0, =__bss_start__
  ldr   r1, =__bss_end__
  ...

.L_call_init:
  ldr   r0, =SystemInit
  blx   r0
  ldr   r0, =__libc_init_array
  blx   r0
  ldr   r0, =main
  blx   r0
```

> **踩坑 1**: Keil 的 scatter file 会自动连接 `__main`，ARMCC 编译的对象直接用。GCC 必须自己写 `.data` 复制和 `.bss` 清零逻辑。  
> **踩坑 2**: 启动文件需要提供 `_init` 和 `_fini` 弱桩函数，否则 `__libc_init_array` 找不到它们会导致链接错误: `undefined reference to '_init'`。  
> **踩坑 3**: VTOR 必须在 Reset_Handler 第一件事就设好 —— 详见第 6.1 节。

### 2.4 链接脚本 — scatter file → ld script 转换

```ld
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}

SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
  .text : { *(.text) *(.text*) } > FLASH
  .rodata : { *(.rodata*) } > FLASH
  .data : {
    PROVIDE(__data_start__ = .);   /* ← VMA 位置 */
    *(.data*)
    PROVIDE(__data_end__ = .);
  } > RAM AT > FLASH

  .bss : {
    PROVIDE(__bss_start__ = .);
    *(.bss*)
    PROVIDE(__bss_end__ = .);
  } > RAM
}
```

> **核心踩坑**: `__data_start__` 的定义位置极其重要 —— 必须在 `.data` 段**内部**（`> RAM` 的上下文中），否则会拿到 Flash LMA 地址，详见第 6.2 节。

---

## 3. 编译器差异导致的语法问题

GCC 和 ARMCC 虽然都遵循 C99，但在语法容忍度上有显著差异。以下是实际遇到的编译错误及其修复：

### 3.1 if-else 不带花括号 → GCC 报错

**现象**: 某些 if-else 嵌套层级多了以后，ARMCC 能正确解析，但 GCC 会警告 `-Wmisleading-indentation` 甚至报错。

**修复文件**: `HardWare/COMMON/at_cmd_common.c`

```c
// ❌ ARMCC 可以，GCC 不行
if (condition)
    do_something();
else if (other)          // GCC: "this 'else' does not guard..."
    do_other();

// ✅ 修复：加花括号
if (condition) {
    do_something();
} else if (other) {
    do_other();
}
```

### 3.2 未定义变量 → GCC 链接报错

**现象**: ARMCC 会优化掉未使用的代码路径，即使有未定义变量也不报错。GCC 会直接报 `undefined reference`。

**修复**: 删除未使用的局部变量声明，或确保所有引用路径中变量都已定义。

### 3.3 隐式函数声明 → GCC 强制要求

**现象**: ARMCC 允许隐式函数声明（返回 int），GCC `-std=c99` 下这是错误。

**修复文件**: `HardWare/I2C/scp_protocol.c`

```c
// 需要显式包含头文件或添加宏定义
#include "gd32f30x.h"
#define SCP_CMD_ENABLE  0x01
```

### 3.4 类型不匹配 — `unsigned int*` vs `uint32_t*`

**现象**: `scp_protocol.h` 中函数声明用了 `unsigned int *`，调用处传入 `uint32_t *`。ARMCC 容忍，GCC 严格地指出类型不兼容。

**修复**: 统一使用 `<stdint.h>` 类型（`uint32_t*`）。

### 3.5 CM_BACKTRACE 汇编文件选择

**现象**: CM_BACKTRACE 自带两份 fault handler 汇编 — `cmb_fault.S`（Keil/armasm）和 `gcc/cmb_fault.S`（GCC）。CMake 中必须显式使用 GCC 版本。

**修复**: `CMakeLists.txt` 中:
```cmake
list(APPEND BACKTRACE_SOURCES
  CM_BACKTRACE/cm_backtrace.c
  CM_BACKTRACE/fault_handler/gcc/cmb_fault.S   # ← GCC 版本!
)
```

### 3.6 CMake 的 `CPU_FLAGS` 分号问题

**现象**: 
```cmake
set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard ...)
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -std=c99")
```
CMake 把 `CPU_FLAGS` 当成列表，展开变成 `-mcpu=cortex-m4;-mthumb;-mfloat-abi=hard;-std=c99`（分号分隔），GCC 不认分号。

**修复**: 用 `string(JOIN)` 先转成空格分隔的字符串:
```cmake
string(JOIN " " CPU_FLAGS_STR ${CPU_FLAGS})
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS_STR} -std=c99")
```

---

## 4. 链接器踩坑

### 4.1 Ninja 找不到 arm-none-eabi-size/objcopy

**现象**: 编译阶段 45/45 通过，但 POST_BUILD 步骤报:
```
系统找不到指定的文件。
```
CMake 传给 Ninja 的 `arm-none-eabi-size` 不包含完整路径，Ninja 的 shell 环境里 PATH 不完整。

**根因**: `set(CMAKE_C_COMPILER arm-none-eabi-gcc)` 只是告诉 CMake 编译器名称，CMake 会用它自己的搜索逻辑找到编译器，但 `CMAKE_SIZE`/`CMAKE_OBJCOPY` 如果也用短名称，Ninja 执行时就会失败。

**修复**: toolchain 文件中使用 `find_program` 获取**完整绝对路径**:
```cmake
find_program(GCC_EXECUTABLE ${TOOLCHAIN_PREFIX}gcc PATHS ${CMAKE_PROGRAM_PATH})
get_filename_component(GCC_DIR ${GCC_EXECUTABLE} DIRECTORY)

function(find_tc_tool VAR NAME)
  set(full_path "${GCC_DIR}/${NAME}")
  if(EXISTS "${full_path}")
    set(${VAR} "${full_path}" PARENT_SCOPE)
  else()
    find_program(${VAR} ${NAME} PATHS ${CMAKE_PROGRAM_PATH})
  endif()
endfunction()

find_tc_tool(CMAKE_C_COMPILER   arm-none-eabi-gcc)
find_tc_tool(CMAKE_OBJCOPY      arm-none-eabi-objcopy)
find_tc_tool(CMAKE_SIZE         arm-none-eabi-size)
```

### 4.2 `_sstack` / `_stext` / `_etext` 未定义

**现象**: CM_BACKTRACE 依赖这些符号记录栈底和代码段边界，如果 linker script 没定义，链接会报 `undefined reference`。

**修复**: 在 linker script 中定义:
```ld
_sstack = ORIGIN(RAM);
_stext  = ORIGIN(FLASH);
/* _etext 在 .text 段末尾: */
.text : { ... _etext = .; } > FLASH
```

### 4.3 `/DISCARD/` 段误删 libc/libgcc

**现象**: linker script 中如果写了:
```ld
/DISCARD/ : {
  *(.text.*)           /* ❌ 这会干掉 libc 的所有 .text.xxx! */
  *(.rodata.*)
}
```
会导致链接失败（找不到 `memset`、`memcpy`、`__aeabi_*` 等）。

**修复**: 删除整个 `/DISCARD/` 段，让 `--gc-sections` 自动处理未使用的段:
```cmake
# 在 toolchain file 中
set(CMAKE_EXE_LINKER_FLAGS_INIT "... -Wl,--gc-sections ...")
```

### 4.4 Windows 下 CMake 输出无 .elf 后缀

**现象**: VSCode tasks 和 launch config 中引用 `build/GD32F303CCT6_Pro.elf`，但文件实际叫 `build/GD32F303CCT6_Pro`（无后缀）。

**根因**: Windows 下 CMake `add_executable` 生成的可执行文件没有 `.elf` 后缀（Linux 有）。

**修复**: 所有引用路径去掉 `.elf`:
```json
// tasks.json: "build/${command:cmake.buildTargetName}"
// launch.json: "executable": "build/${command:cmake.buildTargetName}"
```

---

## 5. OpenOCD 烧录与调试配置

### 5.1 OpenOCD 配置 — 版本差异大坑

使用的 OpenOCD 版本是 sysprogs 0.12.0 fork，语法与官方 openocd 0.12.0 存在差异。

**经历的所有失败尝试:**

| 配置尝试 | 错误信息 | 原因 |
|----------|----------|------|
| `transport select hla_swd` | "doesn't support 'hla_swd'" | sysprogs 版不支持 hla 传输层 |
| `adapter driver stlink` | "adapter driver not found (stlink)" | 只能用 `st-link`（带连字符） |
| `swj_newdap gd32f303cc.dap ...` | "invalid command name 'swj_newdap'" | sysprogs 版没有这个命令 |
| `-chain-position gd32f303cc.dap` | "DAP not found" | 必须先 `dap create` |
| `target create ... -dap ...` | "requires -dap parameter instead of -chain-position" | 语法变了 |

**最终可行的配置** — 直接用 ST 官方配置文件:

```bash
openocd -f interface/stlink.cfg -f target/stm32f3x.cfg \
  -c "program build/GD32F303CCT6_Pro verify reset exit"
```

`stm32f3x.cfg` 能用于 GD32F303CC 是因为两者都是 Cortex-M4 核心，内存布局和 Flash 算法兼容。

### 5.2 烧录命令

```bash
# 在项目根目录执行
openocd -f interface/stlink.cfg -f target/stm32f3x.cfg \
  -c "program build/GD32F303CCT6_Pro verify reset exit"
```

成功输出示例:
```
Info : STLINK V2Jxx (API v2) VID:PID 0483:3748
Info : Target voltage: 3.247339
Info : cortex_m reset_config sysresetreq
Info : gd32f3xx.cpu: Cortex-M4 r0p1 processor detected
target halted due to debug-request, current mode: Thread
** Programming Started **
** Programming Finished **
** Verified OK **
** Resetting Target **
shutdown command invoked
```

### 5.3 VSCode Cortex-Debug 配置

`.vscode/launch.json`:
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Cortex Debug (ST-Link)",
      "type": "cortex-debug",
      "cwd": "${workspaceFolder}",
      "executable": "build/${command:cmake.buildTargetName}",
      "servertype": "openocd",
      "configFiles": [
        "interface/stlink.cfg",
        "target/stm32f3x.cfg"
      ],
      "preLaunchTask": "Build (CMake Debug)",
      "preLaunchCommands": [
        "set {int}0xE000ED08 = 0x08000000"
      ],
      "runToEntryPoint": "main"
    }
  ]
}
```

> **注意**: `preLaunchTask` 名称必须与 `tasks.json` 中的 `label` 完全匹配。CMake Tools 扩展生成的任务名格式为 `"Build (CMake <BuildType>)"`，如 `"Build (CMake Debug)"`。

---

## 6. 运行时崩溃问题的根因分析与修复

这是整个移植过程中最隐蔽、最耗时的部分。

**故障现象**: 烧录成功、复位后，GDB 显示 PC = `0x2000003a`（RAM 地址），xPSR = `0x61000000`，调用栈显示 `g_queue()` → `signal handler` → `0xffffffe8`。

### 6.1 问题一: VTOR 没有被 SYSRESETREQ 清零

**根因**: OpenOCD 执行 "reset" 时使用的是 SYSRESETREQ（系统复位请求），这个软复位**只会复位内核寄存器（R0–R15、xPSR），不会清除 SCB 外设寄存器**。`SCB->VTOR`（地址 `0xE000ED08`）属于系统控制块，不在 SYSRESETREQ 的作用范围内。

之前的固件（可能是在调试过程中）曾将 `VTOR` 设置为 `0x20000000`（RAM 向量表），之后烧录新固件再软复位时，CPU 从 RAM 的 `0x20000000` 读取 SP 和 PC，SP 指向栈顶尚可，但 PC 读出来的是 RAM 中的随机数据（恰好是 `g_queue` 函数的局部变量区）。

**修复 — 两层防御:**

**L1: 启动代码强制初始化**（`startup_gd32f30x_hd_gcc.s`）:
```asm
Reset_Handler:
  /* 必须在任何其他操作之前设置 VTOR */
  ldr   r0, =0xE000ED08    /* SCB->VTOR 寄存器地址 */
  ldr   r1, =0x08000000    /* Flash 基地址 */
  str   r1, [r0]
  dsb                      /* 数据同步屏障 */
  isb                      /* 指令同步屏障 */
  /* 之后才能执行 .data 拷贝等操作 */
```

**L2: GDB preLaunchCommands**（`launch.json`）:
```json
"preLaunchCommands": [
  "set {int}0xE000ED08 = 0x08000000"
]
```
GDB 在 download 之前就先写 VTOR，确保烧录过程中的 halt/reset 不会把 CPU 带到错误位置。

### 6.2 问题二: `__data_start__` 定义在了 Flash LMA

**根因**: 这是链接脚本设计错误。看修复前的代码:

```ld
/* ❌ 错误: . 此时指向的是 LMA (Flash)，因为 .data : AT(...) 还没开始 */
__data_start__ = .;

.data :
{
  . = ALIGN(4);
  *(.data*)
  *(.data*)
  . = ALIGN(4);
  PROVIDE(__data_end__ = .);
} > RAM AT > FLASH
```

在 `.data` 段定义**之前**、**之后**的 `.` 含义完全不同:
- 段**之前**: `.` = 当前输出位置，此时还在 FLASH 中 → `__data_start__` = `0x0800xxxx`
- 段**内部**: 段属性 `> RAM` 生效，`. = ALIGN(4)` 让 `.` 跳到 RAM → VMA = `0x20000000`

结果 startup 代码执行 `ldr r0, =__data_start__` 拿到的就是 Flash 地址，`.data` 段被原地"复制"到 Flash，RAM 中全是垃圾数据——所有带初始值的全局变量（包括 newlib 内部的 `_impure_ptr`）全部是随机值。

**修复后的正确写法**:

```ld
/* ✅ __data_start__ 必须在 .data 段内部定义 */
.data :
{
  . = ALIGN(4);
  PROVIDE(__data_start__ = .);    /* ← 此时 . 在 RAM 中 */
  *(.data*)
  *(.data*)
  . = ALIGN(4);
  PROVIDE(__data_end__ = .);
} > RAM AT > FLASH

/* LMA 用 LOADADDR() 单独获取 */
__data_load__ = LOADADDR(.data);
```

修复前后对比:

| 符号 | 修复前 (错误) | 修复后 (正确) |
|------|--------------|--------------|
| `__data_start__` | `0x08006f4c` (Flash ❌) | `0x20000000` (RAM ✅) |
| `__data_end__` | `0x2000294c` | `0x2000294c` |
| `__data_load__` | `0x08006f4c` | `0x08006fac` |

**调试技巧**: 在 GDB 中查看 `__data_start__` 的值:
```
(gdb) p/x &__data_start__
```
如果返回的是 `0x08xxxxxx`，说明链接脚本写错了。

**总结**: 链接脚本中 `PROVIDE`/`ASSIGN` 的位置决定了符号的实际地址。VMA 相关的符号必须放在**段的内部**定义。

### 6.3 问题三: 缺少 newlib 系统调用

**根因**: Keil 的 microLib 是一个精简 C 库，`printf`/`malloc` 是内部自洽的。GCC 的 newlib-nano 不同——它假设有操作系统，`printf` 内部调用 `_write()`，`malloc` 内部调用 `_sbrk()`。如果这些函数不存在:

- `_write` 缺失: `printf` 根本不会输出到串口，即使你在 `main.c` 里写了 `fputc`。因为 newlib 的 stdio 不调用 `fputc`，它调用 `_write`。
- `_sbrk` 缺失或 stub 太弱: `libnosys.a` 提供了一个空壳 `_sbrk`，没有边界检查。`printf` 首次调用时 newlib 会 malloc 一些内部缓冲区（`__sfp` 结构等），无边界检查的 `_sbrk` 导致堆无限增长，最终覆盖栈区，触发访问非法内存 → HardFault。

**修复**: 新建 `User/syscalls.c`:

```c
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

/* ---- _write: 将 newlib stdio 输出重定向到 USART0 ---- */
extern int fputc(int ch, void *f);

int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++)
        fputc((int)ptr[i], NULL);
    return len;
}

/* ---- _sbrk: 带堆/栈碰撞检测的内存分配 ---- */
extern char _end;      /* linker script 定义的堆起始 */
extern char _estack;   /* linker script 定义的栈底（RAM 顶） */
#define STACK_RESERVED  0x2000   /* 保留 8KB 给栈 */

void *_sbrk(int incr) {
    static char *heap_end = NULL;
    if (heap_end == NULL) heap_end = &_end;

    char *prev = heap_end;
    if (heap_end + incr > (&_estack - STACK_RESERVED)) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end += incr;
    return (void *)prev;
}

/* ---- 其余 syscall 桩 (close/fstat/isatty/lseek/read) ---- */
int _close(int file)      { errno = EBADF; return -1; }
int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(int file)     { return 1; }
off_t _lseek(int file, off_t ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { errno = EBADF; return -1; }
```

**重要**: `printf` 重定向的两层关系:

```
printf() → newlib stdio → _write() → fputc() → USART_DataRegister
                            ▲  syscalls.c ▲    ▲   main.c   ▲
```

Keil microLib 下是 `printf() → fputc()` 的短链路，GCC newlib 则是长链路。只写 `fputc` 是不够的。

---

## 7. 最终的项目文件结构

```
GD32F303CCT6_Pro/
├── GD32F303CC.ld                  # 链接脚本
├── CMakeLists.txt                 # 顶层 CMake 构建
├── cmake/
│   └── arm-gcc-toolchain.cmake    # 交叉编译工具链
├── CMSIS/
│   ├── startup_gd32f30x_hd_gcc.s  # GCC 启动文件
│   ├── system_gd32f30x.c
│   └── core_cm4.h ...
├── FWLIB/                         # 标准外设库 (未改动)
├── HardWare/                      # 硬件驱动层 (少量语法修复)
├── CM_BACKTRACE/                  # 崩溃分析 (选 gcc/ 汇编)
├── User/
│   ├── main.c
│   ├── syscalls.c                 # ★ 新增: newlib 系统调用桩
│   ├── gd32f30x_it.c
│   ├── systick.c
│   └── flash.c
├── .vscode/
│   ├── launch.json                # Cortex-Debug 调试配置
│   ├── tasks.json                 # 构建/烧录任务
│   ├── settings.json              # 工作区设置
│   └── c_cpp_properties.json      # IntelliSense
├── build.bat                      # ★ 新增: 一键编译/烧录脚本
├── build/                         # CMake 构建输出 (gitignore)
└── docs/
    └── Keil_to_GCC_移植指南.md     # ★ 本文档
```

---

## 8. 踩坑速查表

| # | 阶段 | 问题 | 根因 | 修复 |
|---|------|------|------|------|
| 1 | 编译 | CMake flags 包含分号 | `set()` 创建的是 list | `string(JOIN " ")` |
| 2 | 编译 | GCC 语法严格性 | ARMCC 容忍度更高 | 加花括号、显式声明、统一类型 |
| 3 | 编译 | CM_BACKTRACE 编译失败 | 用了 Keil 汇编而非 GCC 版本 | 显式使用 `gcc/cmb_fault.S` |
| 4 | 编译 | `undefined reference to _init` | newlib 需要 `_init/_fini` | 启动文件中添加弱桩函数 |
| 5 | 链接 | Ninja 找不到 size/objcopy | 短名称无 PATH | `find_program` 获取完整路径 |
| 6 | 链接 | `_sstack/_stext/_etext` 未定义 | linker script 缺少 cm_backtrace 符号 | 在 ld 中显式定义 |
| 7 | 链接 | libc/libgcc 符号丢失 | `/DISCARD/` 误删了 `*(.text.*)` | 删除 DISCARD 段，用 `--gc-sections` |
| 8 | 烧录 | OpenOCD 语法不兼容 | sysprogs fork vs 官方差异 | 用 `-f interface/stlink.cfg -f target/stm32f3x.cfg` |
| 9 | 烧录 | Windows 下无 .elf 后缀 | CMake Windows 输出无后缀 | 路径中去掉 `.elf` |
| 10 | 运行 | PC 落在 RAM 区崩溃 | SYSRESETREQ 不清 VTOR | 启动代码首条指令设置 VTOR |
| 11 | 运行 | .data 段未复制到 RAM | `__data_start__` 定义在 Flash LMA | 移到 `.data` 段内部定义 |
| 12 | 运行 | printf 无输出 / HardFault | 缺少 `_write` 和 `_sbrk` | 新建 `syscalls.c` 实现 |
| 13 | 调试 | VSCode 找不到 Build 任务 | preLaunchTask 名称不匹配 | 改为 CMake Tools 生成的精确名称 |

---

## 构建与烧录

### 9.1 build.bat 一键脚本

为了方便日常开发，提供了 `build.bat` 脚本，支持两种模式：

```batch
:: 仅编译
build

:: 编译 + 自动烧录
build flash
```

**脚本内部逻辑**:
```batch
@echo off
setlocal EnableDelayedExpansion

set BUILD_DIR=build
set OPENOCD_INTERFACE=interface/stlink.cfg
set OPENOCD_TARGET=target/stm32f3x.cfg
set BINARY=%BUILD_DIR%\GD32F303CCT6_Pro

:: 创建 build 目录并配置 CMake（首次会自动配置）
if not exist %BUILD_DIR% (
    cmake -B %BUILD_DIR% -G "Ninja" ^
        -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake ^
        -DCMAKE_BUILD_TYPE=Debug
)

:: 编译
cmake --build %BUILD_DIR%
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

:: 如果带 flash 参数，自动烧录
if /I "%~1"=="flash" (
    openocd -f %OPENOCD_INTERFACE% -f %OPENOCD_TARGET% ^
        -c "program %BINARY% verify reset exit"
)
```

> **注意**: Windows PowerShell 中运行脚本需加 `.\` 前缀：`.\build` 或 `.\build flash`。CMD 命令提示符中可直接输入 `build`。

### 9.2 VSCode 集成

| 操作 | 快捷键 | 说明 |
|------|--------|------|
| 仅编译 | `Ctrl+Shift+B` → `Build + Flash` | tasks.json 中配置 |
| 仅烧录 | `Ctrl+Shift+P` → `Run Task` → `Flash Only` | 跳过编译直接烧录 |
| 编译+烧录+调试 | `F5` | Cortex-Debug 全自动 |

### 9.3 手动命令（备用）

```bash
# 配置 (首次)
cmake -B build -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Debug .

# 编译
cmake --build build

# 烧录
openocd -f interface/stlink.cfg -f target/stm32f3x.cfg \
  -c "program build/GD32F303CCT6_Pro verify reset exit"
```

## 10. 运行验证

烧录完成后，通过串口助手（SSCOM）连接板子的 USART0（波特率 115200），可看到固件正常运行：

```
[22:23:26.663]收◆◆wang test33333!
[22:23:27.671]收◆◆wang test33333!
```

**验证要点**:
- `printf` 通过 `_write` → `fputc` → USART0 正确输出
- 串口每 1 秒打印一次，说明 `systick` 定时和主循环正常
- 没有出现 HardFault，程序稳定运行

这表明三个核心修复全部生效：
1. **VTOR 初始化** — 复位后 CPU 从 Flash 正确启动
2. **`.data` 段复制** — 全局变量在 RAM 中有正确初始值
3. **`syscalls.c`** — `printf` 和 `malloc` 的底层支撑到位

---

## 固件占用

```
text: 28592   data: 10572   bss: 12624   total: 51788
Flash: 15% (38.2 / 256 KB)   RAM: 47% (22.7 / 48 KB)
```

---

*文档生成于 2026-08-01，基于实际移植过程中遇到的全部问题及修复方案。*
