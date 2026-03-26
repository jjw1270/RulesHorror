// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_SiteBase.generated.h"


UCLASS(abstract)
class RULESHORROR_API UUI_SiteBase : public UWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	virtual const FString& GetAdditionalSiteAddress();
};
