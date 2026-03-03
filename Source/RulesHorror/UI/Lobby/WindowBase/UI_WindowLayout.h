// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI/Lobby/WindowBase/WindowDefines.h"
#include "UI_WindowLayout.generated.h"

class UClickButton;

UCLASS(abstract)
class RULESHORROR_API UUI_WindowLayout : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<class USizeBox> SizeBox = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> _TitleImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText _TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool _EnableTitleButtons = true;

#pragma region Window Command
public:
	DECLARE_DELEGATE_OneParam(F_OnRequestCommand, EWindowCommand);

	F_OnRequestCommand _OnRequestCommandEvent;

protected:
	UFUNCTION(BlueprintCallable)
	void RequestCommand(EWindowCommand _command);
#pragma endregion Window Command
/////////////////////////////////////////////////////////////////////////////////////
#pragma region Scaler
public:
	void SetSize(const FVector2D& _size);

	UFUNCTION(BlueprintImplementableEvent)
	void EnableWindowScaler(bool _is_enable);

protected:

#pragma endregion Scaler
/////////////////////////////////////////////////////////////////////////////////////
public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnFocused(bool _is_focused);
};
