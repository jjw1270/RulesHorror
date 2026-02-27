// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_WindowLayout.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_WindowLayout : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<class UClickButton> BTN_TitleBar = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> _TitleImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText _TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool _EnableTitleButtons = true;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDM_OnDoubleClickTitleBar);

	UPROPERTY(BlueprintAssignable)
	FDM_OnDoubleClickTitleBar _OnTitleBarDoubleClickedEvent;

protected:
	virtual void NativeOnInitialized() override;

protected:
	UFUNCTION()
	void OnTitleBarButtonDoubleClicked(UClickButton* _btn);
};
