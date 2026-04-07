// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "InteractionSystemDefines.generated.h"

UENUM(BlueprintType)
enum class EInteractionDetectMode : uint8
{
	NA,
	CameraCenter			UMETA(Tooltip = "카메라 중심 기준"),
	Cursor							UMETA(Tooltip = "마우스 커서 기준"),
};

UENUM(BlueprintType)
enum class EInteractionState : uint8
{
	None,
	Detected						UMETA(Tooltip = "인지함"),
	Targeted						UMETA(Tooltip = "상호작용 대상으로 선택됨"),
};
