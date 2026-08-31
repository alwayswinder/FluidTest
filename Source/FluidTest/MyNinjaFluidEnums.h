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
	TouchMultiple = 3 UMETA(DisplayName = "Touch multiple")
};

/**
 * 量化模式（对齐蓝图 QuantizerMode）。
 * 控制纹理偏移的量化策略，用于大世界/远距离流体模拟的性能优化。
 */
UENUM(BlueprintType)
enum class EMyQuantizerMode : uint8
{
	/** 无量化，无纹理偏移 */
	NoQuantizerNoTextureOffset = 0 UMETA(DisplayName = "No Quantizer - No Texture Offset"),
	/** 无量化，手动纹理偏移 */
	NoQuantizerTextureOffsetManuallySet = 1 UMETA(DisplayName = "No Quantizer - Texture Offset Manually Set"),
	/** 无量化，自动纹理偏移（修正极端值） */
	NoQuantizerTextureOffsetAutomaticExtremesCorrected = 2 UMETA(DisplayName = "No Quantizer - Texture Offset Automatic - Extremes Corrected"),
	/** 无量化，自动纹理偏移 */
	NoQuantizerTextureOffsetAutomatic = 3 UMETA(DisplayName = "No Quantizer - Texture Offset Automatic"),
	/** 步长 1 米，自动纹理偏移 */
	Step1mTextureOffsetAutomatic = 4 UMETA(DisplayName = "Step: 1 meter - Texture Offset Automatic"),
	/** 步长 2 米，自动纹理偏移 */
	Step2mTextureOffsetAutomatic = 5 UMETA(DisplayName = "Step: 2 meters - Texture Offset Automatic"),
	/** 步长 3 米，自动纹理偏移 */
	Step3mTextureOffsetAutomatic = 6 UMETA(DisplayName = "Step: 3 meters - Texture Offset Automatic"),
	/** 步长 4 米，自动纹理偏移 */
	Step4mTextureOffsetAutomatic = 7 UMETA(DisplayName = "Step: 4 meters - Texture Offset Automatic"),
	/** 步长 5 米，自动纹理偏移 */
	Step5mTextureOffsetAutomatic = 8 UMETA(DisplayName = "Step: 5 meters - Texture Offset Automatic"),
	/** 步长 10 米，自动纹理偏移 */
	Step10mTextureOffsetAutomatic = 9 UMETA(DisplayName = "Step: 10 meters - Texture Offset Automatic"),
	/** 步长 20 米，自动纹理偏移 */
	Step20mTextureOffsetAutomatic = 10 UMETA(DisplayName = "Step: 20 meters - Texture Offset Automatic"),
	/** 步长 30 米，自动纹理偏移 */
	Step30mTextureOffsetAutomatic = 11 UMETA(DisplayName = "Step: 30 meters - Texture Offset Automatic"),
	/** 步长 50 米，自动纹理偏移 */
	Step50mTextureOffsetAutomatic = 12 UMETA(DisplayName = "Step: 50 meters - Texture Offset Automatic"),
	/** 步长 100 米，自动纹理偏移 */
	Step100mTextureOffsetAutomatic = 13 UMETA(DisplayName = "Step: 100 meters - Texture Offset Automatic"),
	/** 步长 500 米，自动纹理偏移 */
	Step500mTextureOffsetAutomatic = 14 UMETA(DisplayName = "Step: 500 meters - Texture Offset Automatic")
};

/**
 * TraceMesh 的量化移动锁定轴（对齐蓝图 QuantizerAxisIgnore）。
 */
UENUM(BlueprintType)
enum class EMyQuantizerAxisIgnore : uint8
{
	X = 0 UMETA(DisplayName = "X"),
	Y = 1 UMETA(DisplayName = "Y"),
	Z = 2 UMETA(DisplayName = "Z"),
	Camera = 3 UMETA(DisplayName = "CAMERA"),
	None = 4 UMETA(DisplayName = "NONE"),
	All = 5 UMETA(DisplayName = "ALL")
};

/** 模拟精度（对齐蓝图 SimPrecision_Enum）。 */
UENUM(BlueprintType)
enum class EMySimPrecision : uint8
{
	Bit16 = 0 UMETA(DisplayName = "16 bit"),
	Bit32 = 1 UMETA(DisplayName = "32 bit")
};

/** 可导出到外部 RenderTarget 的内部模拟阶段（对齐 RenderTargetList）。 */
UENUM(BlueprintType)
enum class EMyRenderTargetList : uint8
{
	VelocityDensity = 0 UMETA(DisplayName = "RT_VelocityDensity"),
	Divergence = 1 UMETA(DisplayName = "RT_Divergence"),
	Pressure = 2 UMETA(DisplayName = "RT_Pressure"),
	Painter = 4 UMETA(DisplayName = "RT_Painter"),
	Output = 13 UMETA(DisplayName = "RT_Output")
};
