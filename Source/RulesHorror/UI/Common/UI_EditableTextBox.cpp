// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_EditableTextBox.h"
#include "RulesHorrorUtils.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"

void UUI_EditableTextBox::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(IMG_BG))
	{
		IMG_BG->OnMouseButtonDownEvent.BindDynamic(this, &UUI_EditableTextBox::OnClickedBG);
	}

	if (IsValid(EditableTextBox))
	{
		EditableTextBox->OnTextCommitted.RemoveDynamic(this, &UUI_EditableTextBox::OnTextCommitted);
		EditableTextBox->OnTextCommitted.AddDynamic(this, &UUI_EditableTextBox::OnTextCommitted);
		EditableTextBox->SetClearKeyboardFocusOnCommit(true);
	}
}

void UUI_EditableTextBox::InitEditableTextBox(const FVector2D& _size, const FText& _text, const FText& _hint_text, bool _select_all_text_when_focused)
{
	SetEditableTextBoxSize(_size);

	EditableTextBox->SetText(_text);
	EditableTextBox->SetHintText(_hint_text);
	EditableTextBox->SetSelectAllTextWhenFocused(_select_all_text_when_focused);
}

void UUI_EditableTextBox::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsValid(pc))
	{
		_WasShowMouseCursor = pc->ShouldShowMouseCursor();
	}

	EditableTextBox->SetIsEnabled(true);
	EditableTextBox->SetFocus();
}

void UUI_EditableTextBox::OnStartHide_Implementation(EWidgetHideType _hide_type)
{
	Super::OnStartHide_Implementation(_hide_type);

	EditableTextBox->SetIsEnabled(false);

	UWidgetBlueprintLibrary::SetFocusToGameViewport();

	// 마우스를 움직이지 않으면 커서가 숨겨지지 않는 현상 수정
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsValid(pc))
	{
		pc->SetShowMouseCursor(_WasShowMouseCursor);

		if (_WasShowMouseCursor == false)
		{
			float mouse_x, mouse_y;
			if (pc->GetMousePosition(mouse_x, mouse_y))
			{
				pc->SetMouseLocation((int32)mouse_x, (int32)mouse_y);
			}
		}
	}
}

FEventReply UUI_EditableTextBox::OnClickedBG(FGeometry _geometry, const FPointerEvent& _mouse_event)
{
	_OnTextCommittedEvent.Broadcast(EditableTextBox->GetText());
	Close(true);
	return FEventReply(true);
}

void UUI_EditableTextBox::OnTextCommitted(const FText& _text, ETextCommit::Type _commit_method)
{
	if (_commit_method == ETextCommit::OnEnter)
	{
		_OnTextCommittedEvent.Broadcast(EditableTextBox->GetText());
		Close();
	}
}
