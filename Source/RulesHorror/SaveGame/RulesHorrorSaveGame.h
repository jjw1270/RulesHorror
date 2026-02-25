// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RulesHorrorSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API URulesHorrorSaveGame : public USaveGame
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	FString _Nickname;
	
public:
	UFUNCTION(BlueprintCallable)
	bool SetNickName(const FString& _in_nickname, FText& _out_error_text);

public:
	UFUNCTION(BlueprintPure)
	const FString& GetNickName() const { return _Nickname; }
};
