// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RulesHorrorGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API URulesHorrorGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void QuitGame();
};
