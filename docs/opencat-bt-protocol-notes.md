# OpenCat 蓝牙协议速记（Task1）

> 结论基于 `D:/work/OpenCatEsp32-Quadruped-Robot` 当前源码静态分析，未做实机收包。

## 1) 设备名 / 命名来源

- 命名核心来自全局 `uniqueName`，蓝牙实际广播名为 `uniqueName + 后缀`。
- BLE 服务端使用 `"_BLE"` 后缀，经典蓝牙 SSP 使用 `"_SSP"` 后缀。
- `uniqueName` 初始值由机型前缀（如 `Bittle`/`Nybble`）+ 随机十六进制后缀生成，且可通过 `n...` 命令自定义并持久化。

代码证据：
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/configConstants.h`（`getDeviceName()`、`customBleID()`、`uniqueName` 生成逻辑）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/bleUart.h`（`getDeviceName("_BLE")`）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/bluetoothManager.h`（`getDeviceName("_SSP")`）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/reaction.h`（`T_NAME='n'` 分支调用 `customBleID()`）

## 2) 蓝牙类型与源码证据

### 已确认部分

- 固件**同时编译启用**：
  - `BT_BLE`（BLE UART 服务端）
  - `BT_SSP`（经典蓝牙 SPP/SSP 串口）
- BLE 采用 Nordic UART 风格 UUID：
  - Service: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
  - RX(write): `...0002...`
  - TX(notify): `...0003...`
- 经典蓝牙使用 `BluetoothSerial SerialBT`，作为串口输入源之一。

代码证据：
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/OpenCat.h`（`#define BT_BLE`、`#define BT_SSP`）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/bleCommon.h`（UART UUID）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/bleUart.h`（BLE server + characteristic write/notify）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/bluetoothManager.h`（`BluetoothSerial` 与 `blueSspSetup()`）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/moduleManager.h`（`read_serial()` 优先读取 `SerialBT`）

### 待实测部分

- 同一硬件版本上 BLE 与 SSP 是否可“稳定并发双连接”取决于运行时资源与连接时序；源码存在模式切换与互斥处理（尤其 BLE Client/Server 路径），建议实机确认吞吐与稳定性。

## 3) 连接顺序建议（给 xiaozhi 桥接）

1. 扫描两个候选设备名：`<uniqueName>_BLE`、`<uniqueName>_SSP`。
2. 优先尝试 BLE UART（跨平台兼容更好，且有明确 RX/TX UUID）。
3. BLE 失败或不稳定时回退 SSP（经典蓝牙串口）。
4. 连接后先等待约 `1500ms`（`HOLD_TIME`）再发命令；先发轻量探测命令，再发动作命令（避免状态切换影响）。
5. 仅保留一条控制链路发送动作指令，避免多源竞争。

## 3.1) MTU / 最大包长（当前结论）

- **BLE MTU**：源码中未看到显式 `setMTU`，按常见默认实现先按 `ATT_MTU=23` 估算，单包有效负载约 `20` 字节。
- **SPP 包长**：未见固定上限常量，实际受底层缓冲与分包影响，建议桥接层先按 `<=128` 字节单包，超长文本主动分片。
- **落地建议**：
  - 动作命令保持短包（通常 `<16` 字节）；
  - 文本播报先按 `20` 字节分片（BLE）并带发送间隔；
  - 后续通过实测再放宽。
- **待实测**：抓取 BLE 实际协商 MTU 与失败阈值，更新此节。

## 4) 写入格式（是否需要换行/结束符）

### 已确认部分

- OpenCat 命令为：`token(1字节) + payload(可选)`。
- 小写 token（ASCII 命令）：
  - 逻辑终止符是换行 `\n` 或串口超时；
  - 解析后会去掉尾部 `\r/\n`。
- 大写 token（多为二进制/长数据）：
  - 终止符为 `~`。
- BLE 与串口解析**相近但不完全一致**：
  - 二者都根据 token 判定 `\n` 或 `~`；
  - 但串口路径会做尾部 `\r/\n` 清理，BLE 路径无完全等价清理；
  - BLE 还对 `T_TASK_QUEUE` 使用了不同的超时分支。

代码证据：
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/moduleManager.h`（`terminator`、`serialTimeout`、trim `\r\n`）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/bleUart.h`（BLE `onWrite` 使用同类 `terminator` 规则）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/OpenCat.h`（`T_SKILL='k'`、`T_SKILL_DATA='K'`）

### 待实测部分（桥接实现建议）

- 对 `k` 类短指令（如 `kup`、`kwkF`），建议先定统一 SOP：
  - 默认写法：先发**无换行**；
  - 若 `300~500ms` 内无动作/无回包，再追加 `\n` 重试 1 次；
  - 仍失败则判定失败并进入重连或切换链路。
- 对 `K` 等大写数据指令，按协议必须带 `~` 结束。
- `K` 类还需注意边界值风险：源码注释提示 `~` 终止符在特定数据值场景可能存在歧义，桥接层应增加 payload 合法性校验。

## 5) 最小命令格式与首批验证命令

### 最小命令格式

- 技能动作（小写 `k`）：
  - 格式：`k<skillName>[空格<参数>]`
  - 示例：`kup`、`kwkF`、`kvtL 30`
- 命名（`n`）：
  - 格式：`n<deviceName>`
  - 示例：`nMyDog`

证据：
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/OpenCat.h`（`T_SKILL='k'`、`T_NAME='n'`）
- `D:/work/OpenCatEsp32-Quadruped-Robot/src/reaction.h`（`T_SKILL` 解析、`T_NAME` 解析）

## 推荐首批验证命令（3-5 条）

1. `krest`（进入休息姿态，低风险）
2. `kup`（站立）
3. `kwkF`（前进 gait，满足“前进”验证）
4. `kvtL 30`（左转约 30 度，验证带参数解析）
5. `nTestDog`（验证命名持久化逻辑，重启后检查广播名）

> 说明：第 3/4 条动作幅度与持续行为可能受当前姿态、传感器状态、任务队列影响，属“命令已确认、行为效果待实机校准”。

## 6) 未决问题与实机验证步骤

### 未决问题

- BLE 与 SSP 同时连接时，是否会出现 ACK/回包竞争或执行优先级偏置。
- 不同客户端（手机/PC）对“无换行小写命令”的兼容差异。
- 机型差异（Bittle/Nybble 与板卡版本）对默认启用模块和连接稳定性的影响。
- BLE 建连后的固定等待窗口（`HOLD_TIME`）在不同固件版本下是否仍为约 `1500ms`。

### 实机验证步骤（建议）

1. 上电后记录广播名，确认是否为 `<uniqueName>_BLE` 与 `<uniqueName>_SSP`。
2. 仅连 BLE，连接后先等待 `1500ms`，依次发送：`kup`、`kwkF`、`krest`，记录执行与回包。
3. 断 BLE，仅连 SSP，重复同样指令并对比延迟和稳定性。
4. 对 `kup` 做 A/B：无换行 vs `\n`，每种发送 10 次统计成功率，确认桥接默认写法。
5. 发送 `nTestDog`，重启后确认新广播名前缀是否生效。

