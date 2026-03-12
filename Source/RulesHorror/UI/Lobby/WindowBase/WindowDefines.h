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
	NA,
	Minimize							UMETA(ToolTip = "최소화"),
	RestoreSize					UMETA(ToolTip = "사이즈 토글"),
	Close								UMETA(ToolTip = "닫기"),
	Move								UMETA(ToolTip = "창 이동"),
	Resize_North					UMETA(ToolTip = "창 크기 조절"),
	Resize_South					UMETA(ToolTip = "창 크기 조절"),
	Resize_West					UMETA(ToolTip = "창 크기 조절"),
	Resize_East						UMETA(ToolTip = "창 크기 조절"),
	Resize_NorthWest			UMETA(ToolTip = "창 크기 조절"),
	Resize_NorthEast			UMETA(ToolTip = "창 크기 조절"),
	Resize_SouthWest			UMETA(ToolTip = "창 크기 조절"),
	Resize_SouthEast			UMETA(ToolTip = "창 크기 조절"),
	EndDrag							UMETA(ToolTip = "드래그 종료"),
};

UENUM()
enum class EWindowDragType : uint8
{
	NA,
	Move,

	Resize_North,
	Resize_South,
	Resize_West,
	Resize_East,
	Resize_NorthWest,
	Resize_NorthEast,
	Resize_SouthWest,
	Resize_SouthEast,
};
