---
name: mcp-automation-bridge
description: 指导通过 McpAutomationBridge 插件（Unreal MCP，ChiR24/Unreal_mcp）控制本项目的 Unreal 编辑器。涵盖 WebSocket 桥接协议（bridge_hello 握手 + automation_request）与 Native MCP HTTP/SSE 两种传输方式，以及常用工具（manage_level、control_actor、manage_asset 等）的调用格式。当用户要求操控 Unreal 编辑器（打开/保存关卡、生成/移动 Actor、管理资产、截图等）且目标是 McpAutomationBridge 时使用。
---

# McpAutomationBridge（Unreal MCP）使用指导

本 SKILL 针对本项目 `Plugins/McpAutomationBridge` 插件（版本 0.5.30，来自 ChiR24/Unreal_mcp），用于让 AI 通过 MCP 协议控制 **当前正在运行的 Unreal 编辑器实例**。

## 1. 当前环境事实（已验证）

- 项目：`E:\Unreal\Projects\_TEST\FluidTest`，UE 5.7
- 编辑器进程：`UnrealEditor`（FluidTest - Unreal Editor）
- **WebSocket 桥接端口：`8090` / `8091`（127.0.0.1，默认开启，未启用 TLS）**
- Native MCP（HTTP/SSE）：默认端口 `3000`，地址 `http://localhost:3000/mcp`（需在项目设置开启 `Enable Native MCP`，当前环境未开启）
- 测试关卡：`/Game/_MyTest/M_Test`（已验证可打开，16 个 Actor）

## 2. 传输方式

### 方式 A：WebSocket 桥接（推荐，当前已启用）

连接 `ws://127.0.0.1:8091`（或 8090），协议为自定义 JSON 帧：

**第一步：握手（必须）**
```json
{"type": "bridge_hello", "requestId": "hello-1"}
```
成功返回 `bridge_ack`（含 `sessionId`、`protocolVersion: 1`、`capabilities`）。
若插件设置开启了 `Require Capability Token`，握手需带 `"capabilityToken": "<token>"`；错误时返回 `INVALID_CAPABILITY_TOKEN` 并关闭连接（4005）。

**第二步：自动化请求**
```json
{
  "type": "automation_request",
  "requestId": "req-1",
  "action": "manage_level",
  "payload": { "action": "load", "levelPath": "/Game/_MyTest/M_Test" }
}
```
响应为 `automation_response`：
```json
{
  "type": "automation_response",
  "requestId": "req-1",
  "success": true,
  "message": "Level loaded",
  "result": { "loadedPath": "/Game/_MyTest/M_Test", ... }
}
```

**要点：**
- `requestId` 和 `action` 必填，长度 ≤ 128
- 未握手就发请求会收到 `HANDSHAKE_REQUIRED` 错误并断开（4004）
- 每个请求用一个唯一 `requestId`，用于关联响应
- 耗时操作（如 load map）响应可能较慢，需等待完整帧

### 方式 B：Native MCP（Streamable HTTP，需启用）

- 项目设置 → Plugins → MCP Automation Bridge → 勾选 **Enable Native MCP**（默认端口 3000）
- 端点：`POST http://localhost:3000/mcp`，`GET /mcp` 为 SSE 通知流，`DELETE /mcp` 结束会话
- 标准 MCP 流程：`initialize`（返回 `Mcp-Session-Id` 头）→ 后续请求带会话头 → `tools/list` → `tools/call`
- 与 Claude Code / Cursor 等标准 MCP 客户端兼容：
  ```
  claude mcp add unreal-engine --transport http http://localhost:3000/mcp
  ```

## 3. 工具（parent tool / action 结构）

Native MCP 暴露 23 个 canonical 工具；WebSocket 用 `action` 字段 + payload 内的子 `action`。常用工具与子动作：

### manage_level（core 类，打开/保存关卡）
| 子 action | 关键参数 | 说明 |
|---|---|---|
| `load` / `load_level` | `levelPath`（如 `/Game/_MyTest/M_Test`，可省 `.umap`） | 加载关卡（自动保存脏包） |
| `save` / `save_level` | - | 保存当前关卡 |
| `save_as` / `save_level_as` | `savePath` | 另存为 |
| `get_current_level` | - | 获取当前关卡信息（mapName/mapPath/actorCount） |
| `create_level` | `path`, `name` | 新建关卡 |
| `list_levels` | `path` | 列出关卡 |
| `build_lighting` | - | 构建光照 |
| `add_sublevel` | `subLevelPath` | 添加子关卡（流送） |

### control_actor（Actor 控制）
子动作含 spawn / delete / set_transform / set_location / set_rotation / set_scale / add_tag / physics 等。

### manage_asset（资产管理）
子动作含 list / import / duplicate / rename / delete / create_material / get_info 等。

### 其他
`manage_blueprint`（蓝图编辑）、`manage_sequence`（Sequencer/过场）、`control_editor`（PIE/视口/截图）、`manage_ai`、`manage_audio`、`manage_pcg`、`system_control`（控制台命令）、`inspect`（检视）、`execute_python`（Python 执行，代码 ≤ 1MB，写入 Saved/Temp/MCP_Python/）。

## 4. 已验证测试记录（2026 会话）

```
HELLO -> bridge_ack (sessionId=445A70424B2AB783EAB7D29690CA9ED1, protocolVersion=1)
LOAD /Game/_MyTest/M_Test -> success, loadedPath=/Game/_MyTest/M_Test
get_current_level -> mapName=M_Test, mapPath=/Game/_MyTest/M_Test, actorCount=16
```

## 5. 排障

| 现象 | 处理 |
|---|---|
| 连接拒绝 8090/8091 | 确认编辑器运行且插件已启用（Edit → Plugins → MCP Automation Bridge） |
| `HANDSHAKE_REQUIRED` | 先发 bridge_hello |
| `INVALID_CAPABILITY_TOKEN` | 在握手时带上项目设置里的 capability token |
| 插件首次加载失败 | 关闭并重开编辑器（插件重建的已知行为） |
| 3000 端口连接拒绝 | Native MCP 未启用，改用 WebSocket 方式 |
| 构建错误 | 删除 Intermediate/、Binaries/、Saved/ 后重新生成工程并编译 |

## 6. 安全注意事项

- 默认仅绑定 127.0.0.1 回环；开启 `Allow Non-Loopback` 时必须同时开启 `Require Capability Token`
- `execute_python` 有 1MB 代码上限；文件操作有路径穿越防护
- 控制台命令有危险命令校验
