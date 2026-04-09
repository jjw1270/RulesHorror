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

	EditableTextBox->SetFocus();
}

void UUI_EditableTextBox::OnClose_Implementation()
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsValid(pc))
	{
		pc->SetShowMouseCursor(_WasShowMouseCursor);
	}

	Super::OnClose_Implementation();
}

void UUI_EditableTextBox::Close_Implementation(bool _is_skip_anim)
{
	Super::Close_Implementation(_is_skip_anim);

	UWidgetBlueprintLibrary::SetFocusToGameViewport();
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
