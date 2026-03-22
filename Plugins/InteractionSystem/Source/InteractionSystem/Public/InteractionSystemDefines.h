// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "InteractionSystemDefines.generated.h"

UENUM(BlueprintType)
enum class EInteractionState : uint8
{
	None,
	Detected						UMETA(Tooltip = "인지함"),
	Targeted						UMETA(Tooltip = "상호작용 대상으로 선택됨"),
	Interacting					UMETA(Tooltip = "상호작용 중")
};
