// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ItemDeveloperSettings.generated.h"


UCLASS(Config = Game, DefaultConfig)
class ITEMCORE_API UItemDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Config, Category = "Item", meta = (ContentDir, LongPackageName))
	TArray<FDirectoryPath> _ItemTableSearchPaths;
};
