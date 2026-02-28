// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI/Lobby/WindowBase/WindowDefines.h"
#include "UI_WindowLayout.generated.h"

UCLASS(abstract)
class RULESHORROR_API UUI_WindowLayout : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> _TitleImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText _TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool _EnableTitleButtons = true;

public:
	DECLARE_DELEGATE_OneParam(F_OnRequestCommand, EWindowCommand);

	F_OnRequestCommand _OnRequestCommandEvent;

protected:
	UFUNCTION(BlueprintCallable)
	void RequestCommand(EWindowCommand _command);

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnFocused(bool _is_focused);
};
