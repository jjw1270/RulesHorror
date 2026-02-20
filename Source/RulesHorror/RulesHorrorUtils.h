// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUtils.h"
#include "RulesHorrorUtils.generated.h"

UCLASS()
class RULESHORROR_API URulesHorrorUtils : public UCommonUtils
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure, Category = "Utils", meta = (WorldContext = "_obj"))
	static class URulesHorrorGameInstance* GetGameInstance(const UObject* _obj);

	UFUNCTION(BlueprintPure, Category = "Utils", meta = (WorldContext = "_obj"))
	static bool IsInLobby(const UObject* _obj);
};
