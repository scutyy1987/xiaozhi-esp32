# xiaozhi-esp32 ↔ OpenCat 蓝牙桥接实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use @superpowers:subagent-driven-development (recommended) or @superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 `xiaozhi-esp32` 中把对话链路（STT 文本、LLM/TTS 文本）解析为 **OpenCat 固件已内置的技能名**，经 **蓝牙** 下发给 `OpenCatEsp32-Quadruped-Robot`，实现「边播报边做动作」；解析采用 **规则优先 + LLM 结构化兜底 + 本地白名单**，语音采用 **短播报/完整播报混合策略**。

**Architecture:**  
- **双通道**：`动作通道`（用户/助手文本 → 意图 → `k<skill>` 类串口语义）与 `语音通道`（助手回复 → 规划短/全文 → 发往机器狗语音子系统）并行；二者经 **`BtCommandGateway` 串行化队列** 写入蓝牙，避免并发写导致丢包。  
- **白名单**：以 OpenCat 当前编译目标对应的 `Instinct*.h` 中 `skillNameWithType[]` 推导的技能短名表为准（默认 `OpenCatEsp32.ino` 为 `BITTLE` → `../OpenCatEsp32-Quadruped-Robot/src/InstinctBittleESP.h`）。  
- **集成点**：在 `main/application.cc` 的 `protocol_->OnIncomingJson` 回调中，在现有 `tts` / `stt` 处理旁路调用桥接模块（不破坏原有 UI/音频播放逻辑）。

**Tech Stack:** ESP-IDF（C++）、`cJSON`（已有）、NimBLE 或经典蓝牙 SPP（**二选一，见 Task 1 探测结果**）、可选 `esp-nn`/`mbedtls` 无（规则引擎自实现）。

**关联仓库路径：**  
- 小智：`D:\work\xiaozhi-esp32\`  
- OpenCat：`D:\work\OpenCatEsp32-Quadruped-Robot\`（工作区已多根目录挂载）

---

## 0. 前置约束（与已确认需求一致）

| 项 | 选择 |
|----|------|
| 动作范围 | 内置技能全集；组合动作仅固件已集成时映射 |
| 执行策略 | 直接执行（无二次确认） |
| 连接拓扑 | `xiaozhi` 设备直连机器狗 |
| 解析策略 | 混合：规则 → LLM JSON → 白名单 |
| 播报 | 默认短句；用户显式要求「详细说」时全文 |

---

## 1. 文件结构（新建）

| 文件 | 职责 |
|------|------|
| `main/opencat/action_whitelist.h` | 从 `InstinctBittleESP.h` 技能名生成的 **constexpr 列表**或生成脚本输出；`IsAllowedSkill()` |
| `main/opencat/rule_intent_parser.h/.cc` | 中文/英文同义词 → 技能短名（如 `wkF`/`sit`） |
| `main/opencat/llm_intent_parser.h/.cc` | 调用 LLM 要求 **严格 JSON** 输出 `{ "skill": "wkF" \| null, "confidence": 0..1 }`（仅白名单） |
| `main/opencat/speech_planner.h/.cc` | 短播报 vs 全文：关键词 + 长度截断 |
| `main/opencat/opencat_serial_codec.h/.cc` | 将技能名编码为 OpenCat 接受的 **字节串**（与 `reaction.h` / 串口协议一致，通常为 `k` + skill + 可选参数） |
| `main/opencat/bt_command_gateway.h/.cc` | 队列、节流、重连；唯一写 BLE/BT 出口 |
| `main/opencat/opencat_bridge.h/.cc` | 门面：`OnUserText` / `OnAssistantText`；协调双通道 |
| `main/Kconfig.projbuild` 或 `sdkconfig.defaults` 片段 | `CONFIG_OPENCAT_BRIDGE_ENABLE` 等 |

**修改：**

| 文件 | 修改内容 |
|------|----------|
| `main/application.cc` | `InitializeProtocol()` 内 `OnIncomingJson`：在 `stt` / `tts` sentence 分支调用 `OpenCatBridge` |
| `main/CMakeLists.txt` | 加入 `opencat/*.cc`、依赖 `bt`/`nimble` 或 `bt`（按配置） |
| `CMakeLists.txt` / `sdkconfig.defaults` | 打开所选蓝牙栈 |

---

## 2. 任务分解

### Task 1: 协议与传输层探测（OpenCat 侧）

**Files:**  
- Read: `D:\work\OpenCatEsp32-Quadruped-Robot\src\bluetoothManager.h`  
- Read: `D:\work\OpenCatEsp32-Quadruped-Robot\src\bleUart.h`  
- Read: `D:\work\OpenCatEsp32-Quadruped-Robot\src\io.h`  

- [ ] **Step 1: 确认固件对外暴露的蓝牙类型**

用文档记录：机器狗是 **经典 SPP (BluetoothSerial)** 还是 **BLE UART**（`BLEDevice`），还是二者并存。对照 `src/bluetoothManager.h` 中 `SerialBT.begin` 与 `bleUart.h` 的 Service/Characteristic UUID。

- [ ] **Step 2: 抓一条最小命令**

在 OpenCat 串口监视器或官方 App 已验证的指令格式，记录 **「前进」对应的完整字符串**（例如 `k` + `wkF` + 终止符）。参考 `reaction.h` 中 `tQueue->addTask('k', "wkF", ...)` 语义。

- [ ] **Step 3: 输出《对接说明》小节**

写入 `docs/opencat-bt-protocol-notes.md`（本仓库内），包含：UUID/设备名前缀、连接顺序、MTU、最大包长、是否需要 `\n` 结尾。

- [ ] **Step 4: Commit**

```bash
cd docs && git add opencat-bt-protocol-notes.md && git commit -m "docs: OpenCat BLE/SPP protocol notes for xiaozhi bridge"
```

---

### Task 2: 白名单数据与校验

**Files:**  
- Create: `main/opencat/action_whitelist.h`（可由脚本生成）  
- Create: `scripts/gen_opencat_whitelist.py`（可选，从 `InstinctBittleESP.h` 解析 `skillNameWithType[]`）

- [ ] **Step 1: 生成技能短名列表**

规则：对 `skillNameWithType[]` 每项 **去掉最后一个字符**（与 `skill.h` 中 `SkillPreview` 一致）。默认 BITTLE 清单见已整理的 95 项（见对话记录）。

- [ ] **Step 2: 实现 `bool IsAllowedSkill(std::string_view name)`**

- [ ] **Step 3: 编译验证**

Run: `idf.py build`（在 `xiaozhi-esp32` 工程根目录）  
Expected: 成功（若尚未加 CMake 源文件，先仅头文件 + 空 cc 占位）

- [ ] **Step 4: Commit**

```bash
git add main/opencat/action_whitelist.h scripts/gen_opencat_whitelist.py
git commit -m "feat(opencat): add skill whitelist from InstinctBittleESP"
```

---

### Task 3: 串口语义编码（`k` + skill）

**Files:**  
- Create: `main/opencat/opencat_serial_codec.cc`  
- Create: `main/opencat/opencat_serial_codec.h`  

- [ ] **Step 1: 单元逻辑（可先在主机用 g++ 编译小测试）**

```cpp
// 伪代码：与 Task 1 结论一致
std::string EncodeSkillCommand(std::string_view skill) {
  if (!IsAllowedSkill(skill)) return {};
  std::string out;
  out += 'k';
  out.append(skill);
  out += '\n';  // 若协议要求
  return out;
}
```

- [ ] **Step 2: 与 Task 1 文档对齐** 调整终止符与大小写。

- [ ] **Step 3: Commit**

```bash
git add main/opencat/opencat_serial_codec.*
git commit -m "feat(opencat): encode skill commands for serial/BT payload"
```

---

### Task 4: 规则意图解析器（中英同义词）

**Files:**  
- Create: `main/opencat/rule_intent_parser.cc`  
- Create: `main/opencat/rule_intent_parser.h`  

- [ ] **Step 1: 建立第一批词典**（覆盖移动+姿态）

示例映射（可扩展）：  
`前进/往前走` → `wkF`；`后退` → `bkF`；`左转` → `wkL`；`右转` → `wkR`；`停` → `balance` 或文档规定的 `stop` 技能；`坐下` → `sit`；`站立` → `str` 等——**以 OpenCat 文档与实测为准**。

- [ ] **Step 2: 返回结构体 `RuleParseResult { bool hit; std::string skill; }`**

- [ ] **Step 3: Commit**

```bash
git add main/opencat/rule_intent_parser.*
git commit -m "feat(opencat): rule-based keyword intent parser"
```

---

### Task 5: LLM 兜底解析（结构化 JSON）

**Files:**  
- Create: `main/opencat/llm_intent_parser.h/.cc`  

- [ ] **Step 1: 定义系统提示词常量**（英文，节省 token）

内容必须包含：  
- 仅允许 `action_whitelist` 中出现的 `skill`；  
- 无匹配时 `"skill": null`；  
- 禁止解释性文字；  
- `confidence` 浮点。

- [ ] **Step 2: 调用路径**

复用现有 `Application` 与服务器对话能力：若项目已有「向 LLM 发文本」的 API，在 `llm_intent_parser` 内封装；**若无**，则先用 **规则-only MVP**，并把本 Task 标为 Phase 2。

- [ ] **Step 3: 置信度阈值**

默认 `confidence < 0.7` → 不下发动作。

- [ ] **Step 4: Commit**

```bash
git add main/opencat/llm_intent_parser.*
git commit -m "feat(opencat): LLM JSON intent fallback with whitelist gate"
```

---

### Task 6: 播报规划（短/全文）

**Files:**  
- Create: `main/opencat/speech_planner.h/.cc`  

- [ ] **Step 1: 检测「详细/完整说」类触发词**（中文）

- [ ] **Step 2: `PlanSpeech(text) -> { std::string payload; bool full; }`**

- 短播报：截断至 N 字（如 40），可配置。  
- 全文：用户触发或配置项 `CONFIG_OPENCAT_SPEECH_FULL_DEFAULT`。

- [ ] **Step 3: 语音下发编码**

若机器狗侧为 **Petoi 语音模块串口命令**（参考 `OpenCat` 的 `voice.h` / `set_voice`），定义 `EncodeSpeechToDog(...)`；若仅通过 **TTS 音频在 xiaozhi 端播放**、狗端无语音，则本计划需与用户确认——**默认假设**需把文本以 **UTF-8 分包**发到蓝牙（需 Task 1 确认是否支持文本流）。  
若 OpenCat 端仅支持动作串口，则 **MVP 仅在 xiaozhi 端播放 TTS**，狗端只做动作；此项在 Task 1 文档中**必须写死结论**。

- [ ] **Step 4: Commit**

```bash
git add main/opencat/speech_planner.*
git commit -m "feat(opencat): short vs full speech planning"
```

---

### Task 7: 蓝牙命令网关（队列 + 重连）

**Files:**  
- Create: `main/opencat/bt_command_gateway.h/.cc`  

- [ ] **Step 1: 选择栈并实现连接**

- 若 Task 1 为 **BLE UART**：使用 ESP-IDF NimBLE 客户端，订阅/写 Characteristic。  
- 若为 **经典 SPP**：`esp_bt` / `BluetoothSerial` 类库在 ESP-IDF 中的等价实现（查 `xiaozhi` 板级是否已有 BT）。

- [ ] **Step 2: FreeRTOS 队列**

单线程消费写蓝牙，避免与 WiFi/音频任务竞态。

- [ ] **Step 3: 断线重试**

指数退避 + 最大重试次数；日志 `ESP_LOGI/W`。

- [ ] **Step 4: Commit**

```bash
git add main/opencat/bt_command_gateway.*
git commit -m "feat(opencat): BLE/BT command gateway with queue"
```

---

### Task 8: OpenCatBridge 门面与 Application 集成

**Files:**  
- Create: `main/opencat/opencat_bridge.h/.cc`  
- Modify: `main/application.cc`（`InitializeProtocol` 中 `OnIncomingJson`）  
- Modify: `main/CMakeLists.txt`  

- [ ] **Step 1: 在 `OnIncomingJson` 中挂钩**

- `type == "stt"` 且 `text` 存在 → `OpenCatBridge::OnUserText(text)`  
- `type == "tts"` 且 `state == "sentence_start"` → `OpenCatBridge::OnAssistantSentence(text)`  

- [ ] **Step 2: `OnUserText` 内顺序**

`rule` →（未命中）`llm` → `EncodeSkill` → `gateway.enqueue`  

- [ ] **Step 3: `OnAssistantSentence`**

`SpeechPlanner` →（若狗可播文本）`gateway`；否则仅 `display` 已有逻辑 + 本地 TTS。

- [ ] **Step 4: Kconfig 开关**

`CONFIG_OPENCAT_BRIDGE_ENABLE` 关闭时零开销。

- [ ] **Step 5: 编译**

Run: `idf.py build`  
Expected: 成功

- [ ] **Step 6: Commit**

```bash
git add main/application.cc main/CMakeLists.txt main/opencat/
git commit -m "feat(opencat): integrate OpenCat bridge with JSON chat pipeline"
```

---

### Task 9: 验收测试（硬件）

- [ ] **STT 说「前进」**：机器狗在 1s 内执行与 `wkF` 一致动作。  
- [ ] **说「坐下后介绍一下你自己」**：先 `sit`，再听到播报（或 xiaozhi 端 TTS）。  
- [ ] **说未收录技能名**：仅对话，无危险动作。  
- [ ] **蓝牙断开重连**：自动恢复下发。  
- [ ] **长时间对话**：无内存泄漏（`heap` 监控）。

---

## 3. 风险与缓解

| 风险 | 缓解 |
|------|------|
| OpenCat 与 xiaozhi 蓝牙栈冲突（WiFi/BT 共存） | 优先 BLE 与 WiFi 协调；参考 `sdkconfig` 中 coexist；必要时降 WiFi 功率 |
| LLM 幻觉技能名 | 白名单 + `confidence` 阈值 |
| 双通道同时写导致丢包 | 单队列串行化 |
| 语音「狗端播报」协议不明 | Task 1 必须确认；否则 MVP 仅本地 TTS |

---

## 4. Plan Review

- 本计划完成后，应执行 **plan-document-reviewer** 子流程（见 @superpowers:writing-plans），最多 3 轮修订。

---

## 5. 执行交接

**Plan complete and saved to `docs/superpowers/plans/2026-03-23-xiaozhi-opencat-bridge.md`. Two execution options:**

**1. Subagent-Driven (recommended)** — 每个 Task 派生子代理，任务间评审，迭代快  

**2. Inline Execution** — 本会话内按 @superpowers:executing-plans 批量执行并设检查点  

**Which approach?**
