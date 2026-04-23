// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_InitUser.h"
#include "RulesHorrorUtils.h"
#include "WidgetHelper.h"
#include "SaveGameSubsystem.h"
#include "Internationalization/BreakIterator.h"
#include "UI/Common/UI_EditableTextBox.h"


void UUI_InitUser::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	const auto save_game = USaveGameHelper::GetSaveGame(this);
	if (IsValid(save_game))
	{
		save_game->FindSavedStringData(TEXT("Nickname"), _CurrentNickname);
		ShowSetNewNickname(_CurrentNickname.IsEmpty());
	}
}

void UUI_InitUser::OpenEditableTextBoxWidget()
{
	_EditableTextBoxWidget = Cast<UUI_EditableTextBox>(UWidgetHelper::GetRegisteredWidget(this, TEXT("EditableTextBox")));
	if (IsInvalid(_EditableTextBoxWidget))
		return;

	_EditableTextBoxWidget->InitEditableTextBox(_EditableTextBoxWidgetSize, ETB_Nickname->GetText(), ETB_Nickname->GetHintText(), true);

	_EditableTextBoxWidget->_OnTextCommittedEvent.AddDynamic(this, &UUI_InitUser::OnEditableTextBoxWidgetCommitted);

	_EditableTextBoxWidget->_OnHideEvent.AddDynamic(this, &UUI_InitUser::OnHideEditableTextBoxWidget);
	RequestShowMonitorCursor(false);

	_EditableTextBoxWidget->AddToViewport((int32)ERulesHorrorWidgetZOrder::Popup);
}

void UUI_InitUser::OnEditableTextBoxWidgetCommitted(const FText& _text)
{
	ETB_Nickname->SetText(_text);
}

void UUI_InitUser::OnHideEditableTextBoxWidget(UWidgetBase* _widget, EWidgetHideType _hide_type)
{
	if (IsInvalid(_EditableTextBoxWidget))
		return;

	RequestShowMonitorCursor(true);

	_EditableTextBoxWidget->_OnHideEvent.RemoveAll(this);
	_EditableTextBoxWidget->_OnTextCommittedEvent.RemoveAll(this);

	_EditableTextBoxWidget = nullptr;
}

bool UUI_InitUser::SetNewNickname(const FString& _new_nickname, FText& _out_error_text)
{
	const FString prev_nickname = _CurrentNickname;

	const FString nickname = _new_nickname.TrimStartAndEnd();
	if (CheckNickname(nickname, _out_error_text))
	{
		_CurrentNickname = nickname;

		const auto save_game = USaveGameHelper::GetSaveGame(this);
		if (IsValid(save_game))
		{
			save_game->SaveStringData(TEXT("Nickname"), _new_nickname);
			ShowSetNewNickname(_CurrentNickname.IsEmpty());

			if (prev_nickname != _CurrentNickname)
			{
				USaveGameHelper::SaveGame(this);
			}

			return true;
		}
	}

	return false;
}

bool UUI_InitUser::CheckNickname(const FString& _nickname, FText& _out_error_text) const
{
	_out_error_text = FText::GetEmpty();

	if (_nickname.IsEmpty())
	{
		_out_error_text = GETTEXT("ST_Lobby", "Set_Nickname_IsEmpty");
		return false;
	}

	// “사람이 인지하는 글자 수(그래핌)” 기준 길이 계산
	int32 grapheme_count = 0;
	{
		TSharedRef<IBreakIterator> it = FBreakIterator::CreateCharacterBoundaryIterator();
		it->SetString(_nickname);

		int32 start = it->ResetToBeginning();
		for (int32 end = it->MoveToNext(); end != INDEX_NONE; end = it->MoveToNext())
		{
			// [start, end) 가 1개의 사용자 인지 문자(그래핌)로 취급됨
			++grapheme_count;

			// 조기 탈출(최대 길이 초과)
			if (grapheme_count > 16)
				break;

			start = end;
		}
	}

	if (grapheme_count < 2)
	{
		_out_error_text = GETTEXT("ST_Lobby", "Set_Nickname_Too_Short");
		return false;
	}

	if (grapheme_count > 16)
	{
		_out_error_text = GETTEXT("ST_Lobby", "Set_Nickname_Too_Long");
		return false;
	}

	return true;
}