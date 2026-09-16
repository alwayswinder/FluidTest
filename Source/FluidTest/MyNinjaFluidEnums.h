


#pragma once

#include "CoreMinimal.h"
#include "MyNinjaFluidEnums.generated.h"




UENUM(BlueprintType)
enum class EMyUserInput : uint8
{

	None = 0 UMETA(DisplayName = "No user input"),

	MouseSingle = 1 UMETA(DisplayName = "Mouse single"),

	TouchSingle = 2 UMETA(DisplayName = "Touch single"),

	TouchMultiple = 3 UMETA(DisplayName = "Touch multiple")
};





UENUM(BlueprintType)
enum class EMyQuantizerMode : uint8
{

	NoQuantizerNoTextureOffset = 0 UMETA(DisplayName = "No Quantizer - No Texture Offset"),

	NoQuantizerTextureOffsetManuallySet = 1 UMETA(DisplayName = "No Quantizer - Texture Offset Manually Set"),

	NoQuantizerTextureOffsetAutomaticExtremesCorrected = 2 UMETA(DisplayName = "No Quantizer - Texture Offset Automatic - Extremes Corrected"),

	NoQuantizerTextureOffsetAutomatic = 3 UMETA(DisplayName = "No Quantizer - Texture Offset Automatic"),

	Step1mTextureOffsetAutomatic = 4 UMETA(DisplayName = "Step: 1 meter - Texture Offset Automatic"),

	Step2mTextureOffsetAutomatic = 5 UMETA(DisplayName = "Step: 2 meters - Texture Offset Automatic"),

	Step3mTextureOffsetAutomatic = 6 UMETA(DisplayName = "Step: 3 meters - Texture Offset Automatic"),

	Step4mTextureOffsetAutomatic = 7 UMETA(DisplayName = "Step: 4 meters - Texture Offset Automatic"),

	Step5mTextureOffsetAutomatic = 8 UMETA(DisplayName = "Step: 5 meters - Texture Offset Automatic"),

	Step10mTextureOffsetAutomatic = 9 UMETA(DisplayName = "Step: 10 meters - Texture Offset Automatic"),

	Step20mTextureOffsetAutomatic = 10 UMETA(DisplayName = "Step: 20 meters - Texture Offset Automatic"),

	Step30mTextureOffsetAutomatic = 11 UMETA(DisplayName = "Step: 30 meters - Texture Offset Automatic"),

	Step50mTextureOffsetAutomatic = 12 UMETA(DisplayName = "Step: 50 meters - Texture Offset Automatic"),

	Step100mTextureOffsetAutomatic = 13 UMETA(DisplayName = "Step: 100 meters - Texture Offset Automatic"),

	Step500mTextureOffsetAutomatic = 14 UMETA(DisplayName = "Step: 500 meters - Texture Offset Automatic")
};




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


UENUM(BlueprintType)
enum class EMySimPrecision : uint8
{
	Bit16 = 0 UMETA(DisplayName = "16 bit"),
	Bit32 = 1 UMETA(DisplayName = "32 bit")
};


UENUM(BlueprintType)
enum class EMyRenderPipelineMode : uint8
{
	ConsoleVariables = 0 UMETA(DisplayName = "Follow Console Variables"),
	Legacy = 1 UMETA(DisplayName = "Legacy"),
	RDG = 2 UMETA(DisplayName = "RDG")
};


UENUM(BlueprintType)
enum class EMyFluidSimulationBackend : uint8
{
	Material = 0 UMETA(DisplayName = "Material"),
	Compute = 1 UMETA(DisplayName = "Compute")
};


UENUM(BlueprintType)
enum class EMySingleObjectType : uint8
{
	SkeletalMeshBone = 0 UMETA(DisplayName = "Skeletal Mesh Bone"),
	PrimitiveComponent = 1 UMETA(DisplayName = "Primitive Component")
};


UENUM(BlueprintType)
enum class EMyRenderTargetList : uint8
{
	VelocityDensity = 0 UMETA(DisplayName = "RT_VelocityDensity"),
	Divergence = 1 UMETA(DisplayName = "RT_Divergence"),
	Pressure = 2 UMETA(DisplayName = "RT_Pressure"),
	Painter = 4 UMETA(DisplayName = "RT_Painter"),
	Output = 13 UMETA(DisplayName = "RT_Output")
};


UENUM(BlueprintType)
enum class EMyInactiveBehaviour : uint8
{

	HoldLastFrameWhenInactive = 0 UMETA(DisplayName = "Hold last frame when inactive"),

	GrayWhenInactive = 1 UMETA(DisplayName = "Gray when inactive"),

	HiddenWhenInactive = 2 UMETA(DisplayName = "Hidden when inactive")
};
