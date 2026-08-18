---
name: blueprint-to-cpp-migration
description: 蓝图→C++ 迁移指南（FluidTest / UE 5.7）。记录将 FluidNinjaLive 蓝图迁移到 C++ 过程中实测验证的规则与坑点：My 前缀命名规范、蓝图 VM 引用限制、编译要点、迁移步骤。当进行蓝图→C++ 迁移、编写迁移用的 C++ 类/函数/变量、或处理蓝图与 C++ 交互问题时使用。本技能是迁移工作的唯一权威规范，所有迁移代码必须遵守。
---

# 蓝图 → C++ 迁移指南

将 FluidNinjaLive 蓝图迁移到 C++ 的权威规范。所有条目均经编译或运行验证。

## 1. 命名规范（强制规则，必须遵守）

**核心原则：只有类成员变量需要 `My` 前缀（避免与蓝图内现有同名变量冲突）；函数参数与蓝图保持一致，不加前缀。**

| 蓝图 | C++ | 前缀规则 |
|---|---|---|
| 类 `NinjaLiveComponent` | `UMyNinjaLiveComponent` | 类名加 `My` |
| 类 `NinjaLive`（Actor） | `AMyNinjaLiveActor` | 类名加 `My` |
| 函数 `ResetTempArrays` | `MyResetTempArrays()` | 函数名加 `My` |
| 函数 `GetTempArray` | `MyGetTempArray(Index)` | 函数名加 `My`，**参数不加** |
| 函数参数 `FirstIndex` | `FirstIndex`（不变） | **参数不加前缀**，与蓝图一致 |
| 变量 `TempArray0` | `MyTempArray0` | 类成员变量加 `My` |
| 变量 `MapLengthTmp` | `MyMapLengthTmp` | 类成员变量加 `My` |

**规则细则：**
- 类名：C++ 惯例前缀（`U`/`A`）+ `My` + 蓝图名，如 `UMyNinjaLiveComponent`、`AMyNinjaLiveActor`
- 函数名：`My` + 蓝图函数名，如 `MyGetTempArray`、`MyResetTempArrays`
- **函数参数名：不加前缀，与蓝图参数名保持一致**（如 `MyGetTempArray(int32 Index)` 的参数就是 `Index`）
- **类成员变量名：加 `My` 前缀**（避免与蓝图内同名变量冲突），如 `MyTempArray0`、`MyRenderTargetsMap`、`MyMapLengthTmp`
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
void MyAddToTempArray(int32 ArrayIndex, FName Item);  // 内部 GetRef().Add(Item)；参数名与蓝图一致，不加前缀
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

1. **读取蓝图实现**：用 `manage_blueprint` → `get_graph_details`（见 `mcp-automation-bridge` 技能）读取目标函数/变量的完整图结构，不要猜测；导出文本过长被终端截断时，必须按节点名、连接关系分段读取原文件
2. **实现 C++**：按 §1 命名规范，在对应 C++ 父类中实现（类/函数/参数/变量全加 `My` 前缀）
3. **编译验证**：关闭编辑器 → `Build.bat` 编译通过
4. **用户测试**：用户在编辑器里改蓝图（父类继承 / 函数体替换为调用 C++ 版本），确认效果不变
5. **小步增量**：每次只迁移一个函数/变量，迁移一次测试一次，不批量堆叠

### 特殊蓝图节点语义核对（强制）

迁移前必须逐一识别并在 C++ 中保留会改变执行时序、次数或状态的节点；不得因其不是普通计算节点而省略。

- `Delay`、`Retriggerable Delay`、Timeline、异步/Latent 节点：保留延迟、重入和回调时机；可使用 `FTimerManager` 或等价的异步机制，不能改成同步执行。
- `DoOnce`、Gate、FlipFlop、Sequence、循环及重试分支：保留状态、初始状态和 Reset/关闭条件；未连接 Reset 的 `DoOnce` 在同一对象生命周期内只能执行一次。
- `IsValid`、分支、Select：分别保留有效/无效路径、默认值及每个选项的差异，不能只实现主路径。
- 迁移完成后按执行线复核一次：输入、分支、状态变化、潜伏回调、输出副作用均应有对应 C++ 行为。

### 注释规范（重要，必须遵守）

- **注释只写功能说明**：一句话说明该函数/变量是干什么的即可（用途、关键参数/返回值、必要注意事项），**不要大段粘贴蓝图节点还原**（如"已核对完整节点数据：节点46 Add..."这类逐节点描述必须删掉）
- **不确定/不准确没关系**：暂时拿不准的实现细节、含义模糊的参数，可以只写大致功能，甚至留空——随迁移推进逐步补充、完善，不要为了"准确"写长篇注释
- **代码 > 注释**：注释与代码行数应保持合理比例，复杂的蓝图逻辑注释应远少于对应实现代码；能用清晰命名表达的，不重复注释
- 反例：`// 复刻蓝图 xxx（已核对完整节点数据）：节点0 NotEqual → bPickA、节点88 Multiply...`（8 行节点级注释）
- 正例：`/** 处理模拟区域速度：输出 X/Y/Z 分量（Y 取反），CoEff 默认 -0.01 */`

### 架构原则
- **组件持有数据，Actor 通过组件引用访问**：Actor 蓝图里的数组/数据实际属于组件（`GetNinjaLiveComponent() → 组件数据`），Actor 自身不复制数据
- **C++ 只提供父类骨架**：蓝图改继承（Class Settings → Parent Class）由用户在编辑器操作
- 已迁移示例：`ResetTempArrays → MyResetTempArrays`、`GetTempArray → MyGetTempArray`、`MyAddToTempArray` / `MyClearTempArray` / `MyAppendToTempArray`、`CompareMapLength → MyCompareMapLength`、`VelocityHandlerForSimArea → MyVelocityHandlerForSimArea`、`CheckComponentOwner（复合节点）→ MyCheckComponentOwner`、`Enable OWNER Input → MyEnableOwnerInput`

## 5. 相关技能

- 蓝图函数图读取 / MCP 工具调用：见 `mcp-automation-bridge` 技能
- 蓝图资产与关卡操作：见 `special-agent-mcp` 技能
