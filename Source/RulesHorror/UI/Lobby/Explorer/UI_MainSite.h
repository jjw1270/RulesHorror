// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_MainSite.generated.h"


UCLASS(abstract)
class RULESHORROR_API UUI_MainSite : public UWidgetBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetNickName(const FText& _nick_name);
};
