---
name: blueprint-to-cpp-migration
description: 蓝图→C++ 迁移指南（FluidTest / UE 5.7）。记录将 FluidNinjaLive 蓝图迁移到 C++ 过程中实测验证的规则与坑点：My 前缀命名规范、蓝图 VM 引用限制、编译要点、迁移步骤。当进行蓝图→C++ 迁移、编写迁移用的 C++ 类/函数/变量、或处理蓝图与 C++ 交互问题时使用。本技能是迁移工作的唯一权威规范，所有迁移代码必须遵守。
---

# 蓝图 → C++ 迁移指南

将 FluidNinjaLive 蓝图迁移到 C++ 的权威规范。所有条目均经编译或运行验证。

## 1. 命名规范（强制规则，必须遵守）

**所有迁移到 C++ 的类名、函数名、参数名、变量名都必须添加 `My` 前缀**，避免与蓝图内同名类型/变量混淆：

| 蓝图 | C++（带 My 前缀） |
|---|---|
| 类 `NinjaLiveComponent` | `UMyNinjaLiveComponent` |
| 类 `NinjaLive`（Actor） | `AMyNinjaLiveActor` |
| 函数 `ResetTempArrays` | `MyResetTempArrays()` |
| 函数 `GetTempArray` | `MyGetTempArray(Index)` |
| 函数参数 `FirstIndex` | `MyFirstIndex`（参数名同样加前缀） |
| 变量 `TempArray0` | `MyTempArray0` |
| 变量 `MapLengthTmp` | `MyMapLengthTmp` |

**规则细则：**
- 类名：C++ 惯例前缀（`U`/`A`）+ `My` + 蓝图名，如 `UMyNinjaLiveComponent`
- 函数名：`My` + 蓝图函数名，如 `MyGetTempArray`
- **参数名**：`My` + 蓝图参数名，如 `MyFirstIndex`、`MyLastIndex`
- **变量名**：`My` + 蓝图变量名，如 `MyTempArray0`、`MyRenderTargetsMap`
- Category 统一用 `FluidSim|Temp`（或对应领域）
- 迁移前必须确认蓝图原名（用 `manage_blueprint` 读取，见 `mcp-automation-bridge` 技能）

## 2. 蓝图 VM 引用限制（已实测验证，重要）

### 现象
UFUNCTION 即使写成返回引用，在**蓝图层**仍然按值拷贝。

### 证据
```cpp
// C++ 可以编译（引用返回合法）
UFUNCTION(BlueprintPure)
TArray<FName>& MyGetTempArray(int32 Index);
```
但 UHT 生成的 thunk 参数结构是**值类型**：
```cpp
struct MyNinjaLiveComponent_eventMyGetTempArray_Parms
{
    int32 Index;
    TArray<FName> ReturnValue;  // ← 值，不是引用！
};
```
蓝图虚拟机（Blueprint VM）**没有"返回引用"概念**——所有函数返回值在 VM 层一律值拷贝。蓝图里 `MyGetTempArray(0) → Array_Add` 修改的是临时副本，组件数组不变（表现为"组件数组始终为空"）。

### 两层语义对比
| 调用方式 | 返回引用是否生效 |
|---|---|
| C++ 内部调用 | ✅ 生效（语言语义） |
| 蓝图调用 | ❌ 被 thunk 强制按值拷贝 |

### 正确模式
需要"蓝图修改数组/对象内部数据"时，两种做法：

**做法 A：封装 BlueprintCallable 操作函数（内部用引用完成修改）**
```cpp
UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
void MyAddToTempArray(int32 MyArrayIndex, FName MyItem);  // 内部 GetRef().Add(MyItem)
```

**做法 B：直接用 BlueprintReadWrite 变量**
数组/对象变量用 `UPROPERTY(BlueprintReadWrite)`，蓝图里 Get/Set 变量是**引用语义**（Array_Add 直接改变量本身）。

### 判断标准
- 返回**值**（读数据）：`BlueprintPure` + 值返回
- 修改**内部数据**（写）：`BlueprintCallable` 操作函数 或 `BlueprintReadWrite` 变量
- 永不依赖"蓝图调用返回引用写回"

## 3. UE 编译要点（本项目实测）

- **UE 5.7 没有 `RenderUtils` 模块**（旧版名称）：渲染相关只需 `RenderCore` + `RHI`
- **UHT 枚举要求**：`UPROPERTY` 成员用旧式 `enum`（如 `ETextureRenderTargetFormat`）必须包 `TEnumAsByte<...>`；C++11 enum class 需显式底层类型（`uint8`）
- **include 路径**：`FluidTest.Shared.rsp` 的 include path 是 `<project>/Source`（模块根），所以 `#include "FluidSim/X.h"` 解析为 `Source/FluidSim/X.h`（不存在）——**必须写 `"FluidTest/FluidSim/X.h"`**（相对 Source 根）
- **编辑器运行中无法命令行编译**（Live Coding 激活）：需先关编辑器再 `Build.bat`
- **UCLASS 需 Blueprintable**：仅 `UCLASS()` 的类能被放置但**不能作为蓝图父类**；加 `UCLASS(Blueprintable, BlueprintType)` 后蓝图才能继承

## 4. 迁移流程（必须遵守）

1. **读取蓝图实现**：用 `manage_blueprint` → `get_graph_details`（见 `mcp-automation-bridge` 技能）读取目标函数/变量的完整图结构，不要猜测
2. **实现 C++**：按 §1 命名规范，在对应 C++ 父类中实现（类/函数/参数/变量全加 `My` 前缀）
3. **编译验证**：关闭编辑器 → `Build.bat` 编译通过
4. **用户测试**：用户在编辑器里改蓝图（父类继承 / 函数体替换为调用 C++ 版本），确认效果不变
5. **小步增量**：每次只迁移一个函数/变量，迁移一次测试一次，不批量堆叠

### 架构原则
- **组件持有数据，Actor 通过组件引用访问**：Actor 蓝图里的数组/数据实际属于组件（`GetNinjaLiveComponent() → 组件数据`），Actor 自身不复制数据
- **C++ 只提供父类骨架**：蓝图改继承（Class Settings → Parent Class）由用户在编辑器操作
- 已迁移示例：`ResetTempArrays → MyResetTempArrays`、`GetTempArray → MyGetTempArray`、`MyAddToTempArray` / `MyClearTempArray` / `MyAppendToTempArray`

## 5. 相关技能

- 蓝图函数图读取 / MCP 工具调用：见 `mcp-automation-bridge` 技能
- 蓝图资产与关卡操作：见 `special-agent-mcp` 技能
