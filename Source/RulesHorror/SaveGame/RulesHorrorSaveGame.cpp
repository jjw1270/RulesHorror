// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGame/RulesHorrorSaveGame.h"
#include "UI/RulesHorrorWidgetHelper.h"
#include "Internationalization/BreakIterator.h"

bool URulesHorrorSaveGame::SetNickName(const FString& _in_nickname, FText& _out_error_text)
{
	_out_error_text = FText::GetEmpty();

	// 양끝 공백 제거
	const FString nickname = _in_nickname.TrimStartAndEnd();

	if (nickname.IsEmpty())
	{
		_out_error_text = GETTEXT("ST_Lobby", "Set_Nickname_IsEmpty");
		return false;
	}

	// “사람이 인지하는 글자 수(그래핌)” 기준 길이 계산
	int32 grapheme_count = 0;
	{
		TSharedRef<IBreakIterator> it = FBreakIterator::CreateCharacterBoundaryIterator();
		it->SetString(nickname);

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

	_Nickname = nickname;
	return true;
}
