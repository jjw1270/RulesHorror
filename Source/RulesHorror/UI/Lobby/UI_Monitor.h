// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBase.h"
#include "UI_Monitor.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_Monitor : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidgetSwitcher> WidgetSwitcher = nullptr;
	
protected:
	static int32 _LastActiveWidgetIndex;

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnShow_Implementation() override;

#if WITH_EDITOR
public:
	static void ResetLastActiveWidgetindex() { _LastActiveWidgetIndex = -1; }
#endif

protected:
	UFUNCTION(BlueprintCallable)
	void SetActiveWidgetAndShow(UWidgetBase* _widget, bool _is_skip_anim);
};
