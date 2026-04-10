// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBase.h"
#include "Components/EditableTextBox.h"
#include "UI_EditableTextBox.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_EditableTextBox : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UImage> IMG_BG = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> EditableTextBox = nullptr;

	bool _WasShowMouseCursor = true;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDM_OnTextCommitted, const FText&, _text);

	UPROPERTY(BlueprintAssignable, BlueprintReadOnly)
	FDM_OnTextCommitted _OnTextCommittedEvent;

public:
	UFUNCTION(BlueprintCallable)
	void InitEditableTextBox(const FVector2D& _size, const FText& _text, const FText& _hint_text, bool _select_all_text_when_focused);

protected:
	virtual void NativeConstruct() override;
	virtual void OnShow_Implementation() override;
	virtual void OnClosing_Implementation() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetEditableTextBoxSize(const FVector2D& _size);

	UFUNCTION()
	FEventReply OnClickedBG(FGeometry _geometry, const FPointerEvent& _mouse_event);

	UFUNCTION()
	void OnTextCommitted(const FText& _text, ETextCommit::Type _commit_method);

public:
	UFUNCTION(BlueprintPure)
	UEditableTextBox* GetEditableTextBox() const { return EditableTextBox; }
};
