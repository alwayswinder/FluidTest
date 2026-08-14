// MyNinjaFluidEnums.h — 对齐 FluidNinjaLive 蓝图自定义枚举
// 复刻自 /Game/FluidNinjaLive/Core/UserInput_Enum 等枚举资产。
// 命名带 My 前缀避免与蓝图内同名类型冲突（见 blueprint-to-cpp-migration 技能）。

#pragma once

#include "CoreMinimal.h"
#include "MyNinjaFluidEnums.generated.h"

/**
 * 用户输入方式（复刻蓝图 UserInput_Enum）。
 * 数值映射（用户提供显示名 + MCP 实测 MOUSE_SINGLE:1 交叉验证）：
 *   NewEnumerator0 = No user input（数值 0）
 *   NewEnumerator1 = Mouse single（数值 1）
 *   NewEnumerator2 = Touch single（数值 2）
 *   NewEnumerator4 = Touch multiple（数值 4；NewEnumerator3 已从蓝图删除）
 * 蓝图中的显示名通过 UMETA(DisplayName=...) 与蓝图 UserInput_Enum 保持一致。
 */
UENUM(BlueprintType)
enum class EMyUserInput : uint8
{
	/** No user input（蓝图 NewEnumerator0，数值 0）—— 无输入 */
	None = 0 UMETA(DisplayName = "No user input"),
	/** Mouse single（蓝图 NewEnumerator1，数值 1）—— 鼠标单点 */
	MouseSingle = 1 UMETA(DisplayName = "Mouse single"),
	/** Touch single（蓝图 NewEnumerator2，数值 2）—— 触摸单点 */
	TouchSingle = 2 UMETA(DisplayName = "Touch single"),
	/** Touch multiple（蓝图 NewEnumerator4，数值 4）—— 触摸多点 */
	TouchMultiple = 4 UMETA(DisplayName = "Touch multiple")
};
