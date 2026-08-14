---
name: special-agent-mcp
description: 指导通过 SpecialAgent 插件（Artisan Gameworks，MCP Server for UE5）控制本项目的 Unreal 编辑器。该插件提供 34 个标准 MCP 工具（标准 HTTP/SSE + JSON-RPC 2.0），包括全量 Python 执行（unreal 模块）、关卡/世界操作、资产搜索、视口截图、光照/植被/地形等关卡设计工具。当用户要求操控 Unreal 编辑器且目标是 SpecialAgent（如执行 Python、截图、摆放物体、关卡设计）时使用。
---

# SpecialAgent MCP 使用指导

本 SKILL 针对本项目 `Plugins/SpecialAgentPlugin-main` 插件（SpecialAgent，版本 0.1.0，Artisan Gameworks），用于让 AI 通过标准 MCP 协议控制 **当前正在运行的 Unreal 编辑器实例**。

## 1. 当前环境事实（已验证）

- 项目：`E:\Unreal\Projects\_TEST\FluidTest`，UE 5.7
- 编辑器进程：`UnrealEditor`（FluidTest - Unreal Editor）
- **MCP 服务器端口：`8767`**（127.0.0.1，默认开启，HTTP/SSE 原生传输）
- 端点：
  - `POST http://localhost:8767/mcp`（Streamable HTTP 主端点）
  - `GET/POST http://localhost:8767/sse`（SSE 传输）
  - `POST http://localhost:8767/message`（消息端点）
  - `GET http://localhost:8767/health`（健康检查）
- 健康检查已验证：`{"status":"healthy","server":"SpecialAgent MCP Server","version":"1.0.0","port":8767}`
- 测试关卡：`/Game/_MyTest/M_Test`（已验证，16 个 Actor）
- 配置：`Plugins/SpecialAgentPlugin-main/Config/DefaultSpecialAgent.ini`（端口 8767、`PythonSandboxEnabled=true`、`AllowedPythonModules=unreal,math,json,random`、执行超时 30s）

## 2. 调用方式（标准 MCP，JSON-RPC 2.0）

**initialize（可选，服务器也接受直接调用）**
```json
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"client","version":"1.0"}}}
```
返回 `protocolVersion` 与 `instructions`（含工作流提示：先 screenshot 再 trace 再操作）。

**tools/list**
```json
{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}
```
返回 34 个工具（`result.tools[]`，每项含 name/description/inputSchema）。

**tools/call**
```json
{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"python-execute","arguments":{"code":"..."}}}
```
工具结果在 `result.content[]`（`type: text`，内容为 JSON 字符串，含 `success`/`stdout`/`stderr` 等）；部分工具返回 `type: image`（PNG 截图）。

## 3. 工具分类（34 个）

| 类别 | 工具 | 用途 |
|---|---|---|
| **Python（核心）** | `python-execute` | 执行任意 Python，完整访问 `unreal` 模块（UE5 Python API） |
| | `python-execute_file` | 执行 Content/Python 目录下的 .py 脚本 |
| | `python-list_modules` | 列出可用模块和脚本 |
| **截图/视口** | `screenshot-capture` | 截取视口图（默认 1280x720，返回 image；quality=100 为无损 PNG）——**工作流第一步** |
| | `screenshot-save` | 截图保存到文件 |
| | `viewport-set_location` / `set_rotation` / `get_transform` / `focus_actor` | 视口相机控制 |
| | `viewport-trace_from_screen` | 从屏幕坐标（0-1 百分比）获取 3D 位置与表面法线——**放置物体前必用** |
| **世界/关卡** | `world-list_actors` / `get_actor` / `find_actors_by_tag` / `find_actors_in_radius` | 查询 Actor |
| | `world-spawn_actor` / `delete_actor` / `set_actor_location` / `set_actor_rotation` / `set_actor_scale` / `duplicate_actor` | Actor 操作 |
| | `world-get_level_info` | 当前关卡信息（level_name/level_path/actor_count/bounds） |
| **资产** | `assets-list` / `assets-find` / `assets-search` / `assets-get_properties` / `assets-get_bounds` / `assets-get_info` | 内容浏览器搜索与检视（get_bounds/get_info 在摆放前获取尺寸与信息） |
| **关卡设计** | `landscape`、`foliage`、`lighting`、`streaming`、`navigation`、`performance`（README 提及 71+ 工具，当前注册 34 个） | 地形雕刻、植被、灯光、子关卡流送、NavMesh、性能统计 |
| **工具类** | `utility-save_level` / `utility-undo` / `utility-redo` / `utility-select_actor` / `utility-get_selection` / `utility-get_selection_bounds` / `utility-select_at_screen` | 保存/撤销/选择 |

## 4. 已验证测试记录（2026 会话）

```
GET /health -> {"status":"healthy","server":"SpecialAgent MCP Server","version":"1.0.0","port":8767,"running":true}
initialize -> protocolVersion 2024-11-05, instructions 含工作流提示
tools/list -> 34 tools
tools/call python-execute:
  unreal.EditorLevelLibrary.load_level('/Game/_MyTest/M_Test')
  -> {"success":true,"stdout":"SpecialAgent test: current level = /Game/_MyTest/M_Test.M_Test","execution_time":0.25}
tools/call world-get_level_info -> level_name=M_Test, level_path=/Game/_MyTest/M_Test.M_Test, actor_count=16
```

## 5. 推荐工作流（插件 instructions 内置建议）

1. **screenshot-capture** → 看到当前视口
2. **viewport-trace_from_screen**（或 select_at_screen）→ 获取放置点 3D 位置和朝向
3. 执行操作（spawn/修改/缩放等，一次放一个再截图确认）
4. **screenshot-capture** → 验证结果，迭代

屏幕坐标约定：所有屏幕类工具用 0-1 百分比（0.5,0.5=中心；0.25,0.75=左 25%、上 75%）。

## 6. 排障

| 现象 | 处理 |
|---|---|
| /health 失败 | 确认编辑器运行、插件启用（Edit → Plugins → SpecialAgent）、端口 8767 未被占用 |
| 工具不出现 | 调 `tools/list` 验证注册；查编辑器 Output Log（LogSpecialAgent） |
| Python 超时 | 默认 30s；调大 `timeout` 参数或改 ini 中 `PythonExecutionTimeout` |
| Python 模块被拒 | 沙箱只允许 `AllowedPythonModules=unreal,math,json,random`，需在 ini 添加 |
| 客户端连不上 | 某些 IDE 需在编辑器启动后再启动客户端（连接只在客户端启动时尝试） |
| 端口冲突 | 改 `Config/DefaultSpecialAgent.ini` 的 `ServerPort` |

## 7. 安全说明

- 默认仅本机回环访问；`PythonSandboxEnabled=true` 限制可导入模块
- `python-execute` 拥有完整 `unreal` 模块权限，等于"编辑器能做的它都能做"——执行破坏性操作前先确认
