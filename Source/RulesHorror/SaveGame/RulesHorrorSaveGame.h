// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CustomSaveGame.h"
#include "RulesHorrorSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API URulesHorrorSaveGame : public UCustomSaveGame
{
	GENERATED_BODY()

public:
	virtual void ClearData() override;
};
