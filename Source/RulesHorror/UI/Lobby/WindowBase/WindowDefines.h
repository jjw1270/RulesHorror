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
	Resize_UpDown				UMETA(ToolTip = "상하 크기 조절"),
	Resize_LeftRight			UMETA(ToolTip = "좌우 크기 조절"),
	Resize_SouthEast			UMETA(ToolTip = "우하 크기 조절"),
	Resize_SouthWest			UMETA(ToolTip = "우상 크기 조절"),
	EndDrag							UMETA(ToolTip = "드래그 종료"),
};

UENUM()
enum class EWindowDragType : uint8
{
	NA,
	Move,
	Resize_UpDown,
	Resize_LeftRight,
	Resize_SouthEast,
	Resize_SouthWest,
};
