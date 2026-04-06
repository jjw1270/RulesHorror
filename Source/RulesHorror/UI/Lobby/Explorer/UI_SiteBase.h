// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_SiteBase.generated.h"


UCLASS(abstract)
class RULESHORROR_API UUI_SiteBase : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	FString _AdditionalSiteAddress = FString();

public:
	UFUNCTION(BlueprintCallable)
	void SetAdditionalSiteAddress(const FString& _new_address);

	const FString& GetAdditionalSiteAddress() const { return _AdditionalSiteAddress; }
};
