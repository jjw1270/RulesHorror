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
};
