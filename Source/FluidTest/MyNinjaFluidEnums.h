// MyNinjaFluidEnums.h — FluidNinjaLive 蓝图自定义枚举对应的 C++ 枚举
// 数值映射与蓝图 UserInput_Enum 保持一致（已实测交叉验证）。

#pragma once

#include "CoreMinimal.h"
#include "MyNinjaFluidEnums.generated.h"

/**
 * 用户输入方式（对齐蓝图 UserInput_Enum）。
 */
UENUM(BlueprintType)
enum class EMyUserInput : uint8
{
	/** 无输入 */
	None = 0 UMETA(DisplayName = "No user input"),
	/** 鼠标单点 */
	MouseSingle = 1 UMETA(DisplayName = "Mouse single"),
	/** 触摸单点 */
	TouchSingle = 2 UMETA(DisplayName = "Touch single"),
	/** 触摸多点 */
	TouchMultiple = 4 UMETA(DisplayName = "Touch multiple")
};
