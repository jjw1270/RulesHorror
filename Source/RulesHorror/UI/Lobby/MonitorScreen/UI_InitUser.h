// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Lobby/MonitorScreen/UI_MonitorScreenWidget.h"
#include "UI_InitUser.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_InitUser : public UUI_MonitorScreenWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> ETB_Nickname = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString _CurrentNickname;

	UPROPERTY(EditAnywhere)
	FVector2D _EditableTextBoxWidgetSize = FVector2D(480.0f, 52.0f);

	UPROPERTY()
	TObjectPtr<class UUI_EditableTextBox> _EditableTextBoxWidget = nullptr;

protected:
	virtual void OnShow_Implementation() override;

protected:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ShowSetNewNickname(bool _show);

	UFUNCTION(BlueprintCallable)
	void OpenEditableTextBoxWidget();

	UFUNCTION()
	void OnEditableTextBoxWidgetCommitted(const FText& _text);

	UFUNCTION()
	void OnCloseEditableTextBoxWidget(UWidgetBase* _widget);

	UFUNCTION(BlueprintCallable)
	bool SetNewNickname(const FString& _new_nickname, FText& _out_error_text);

	bool CheckNickname(const FString& _nickname, FText& _out_error_text) const;
};
