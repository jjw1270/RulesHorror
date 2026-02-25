// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGame/RulesHorrorSaveGame.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGameSubsystem.generated.h"

/*
 * 
 */
UCLASS()
class RULESHORROR_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

friend class USaveGameHelper;
	
protected:
	const FString _SaveGameSlotName = TEXT("SaveGameSlot");

	UPROPERTY()
	TObjectPtr<URulesHorrorSaveGame> _SaveGame = nullptr;

public:
	virtual void Initialize(FSubsystemCollectionBase& _collection) override;

protected:
	void SaveGameToSlot();
};

UCLASS()
class RULESHORROR_API USaveGameHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"))
	static URulesHorrorSaveGame* GetSaveGame_Editable(const UObject* _world_ctx);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"))
	static const URulesHorrorSaveGame* GetSaveGame(const UObject* _world_ctx);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"))
	static void SaveGame(const UObject* _world_ctx);
};
