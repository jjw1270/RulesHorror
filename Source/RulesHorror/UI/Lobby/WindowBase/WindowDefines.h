// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WindowDefines.generated.h"


UENUM(BlueprintType)
enum class EWindowWidgetType : uint8
{
	NA = 0,
	Explorer,
	ControlPanel
};

UENUM(BlueprintType)
enum class EWindowCommand : uint8
{
	Minimize						UMETA(ToolTip = "최소화"),
	RestoreSize				UMETA(ToolTip = "사이즈 토글"),
	Close							UMETA(ToolTip = "닫기"),
	StartDrag					UMETA(ToolTip = "창 이동"),
};
