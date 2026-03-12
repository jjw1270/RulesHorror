// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedDragDropOperation.h"
#include "UI/Lobby/WindowBase/WindowDefines.h"
#include "WindowDragDropOperation.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API UWindowDragDropOperation : public UAdvancedDragDropOperation
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite)
	EWindowDragType _DragType = EWindowDragType::NA;

	UPROPERTY()
	FVector2D _InitialWindowPos = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D _InitialWindowSize = FVector2D::ZeroVector;
};
