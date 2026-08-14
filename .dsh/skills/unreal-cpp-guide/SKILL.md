---
name: unreal-cpp-guide
description: Unreal Engine C++ 开发指南，记录本项目（FluidTest / UE 5.7）在蓝图迁移到 C++ 过程中验证过的关键知识与坑点。包括蓝图 VM 引用限制、数组/Map 迁移模式、蓝图函数图读取等。当进行 UE C++ 编码、蓝图→C++ 迁移、或处理蓝图与 C++ 交互问题时使用。
---

# Unreal C++ 开发指南

记录本项目在"蓝图→C++ 复刻"过程中**实测验证**的关键知识。所有条目均经过编译或运行验证。

## 1. 蓝图 VM 引用限制（已实测验证，重要）

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
void MyAddToTempArray(int32 ArrayIndex, FName Item);  // 内部 GetRef().Add(Item)
```

**做法 B：直接用 BlueprintReadWrite 变量**
数组/对象变量用 `UPROPERTY(BlueprintReadWrite)`，蓝图里 Get/Set 变量是**引用语义**（Array_Add 直接改变量本身）。

### 判断标准
- 返回**值**（读数据）：`BlueprintPure` + 值返回
- 修改**内部数据**（写）：`BlueprintCallable` 操作函数 或 `BlueprintReadWrite` 变量
- 永不依赖"蓝图调用返回引用写回"

## 2. UE 编译要点（本项目实测）

- **UE 5.7 没有 `RenderUtils` 模块**（旧版名称）：渲染相关只需 `RenderCore` + `RHI`
- **UHT 枚举要求**：`UPROPERTY` 成员用旧式 `enum`（如 `ETextureRenderTargetFormat`）必须包 `TEnumAsByte<...>`；C++11 enum class 需显式底层类型（`uint8`）
- **include 路径**：`FluidTest.Shared.rsp` 的 include path 是 `<project>/Source`（模块根），所以 `#include "FluidSim/X.h"` 解析为 `Source/FluidSim/X.h`（不存在）——**必须写 `"FluidTest/FluidSim/X.h"`**（相对 Source 根）
- **编辑器运行中无法命令行编译**（Live Coding 激活）：需先关编辑器再 `Build.bat`
- **UCLASS 需 Blueprintable**：仅 `UCLASS()` 的类能被放置但**不能作为蓝图父类**；加 `UCLASS(Blueprintable, BlueprintType)` 后蓝图才能继承

## 3. 蓝图→C++ 迁移模式（本项目方法）

- **小步增量**：每次只迁移一个蓝图函数/变量到 C++，迁移一次测试一次
- **命名带 My 前缀**：避免与蓝图内同名类型混淆（如 `UMyNinjaLiveComponent`、`MyGetTempArray`）
- **C++ 只提供父类骨架**：蓝图改继承（Class Settings → Parent Class）由用户在编辑器操作
- **组件持有数据，Actor 通过组件引用访问**：Actor 蓝图里的数组实际属于组件（`GetNinjaLiveComponent() → 组件数组`），Actor 自身不复制数组

## 4. 相关技能

- 蓝图函数图读取 / MCP 工具调用：见 `mcp-automation-bridge` 技能
