// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/Page_InitUser.h"
#include "SaveGame/SaveGameSubsystem.h"

void UPage_InitUser::NativeConstruct()
{
	Super::NativeConstruct();

	const auto save_game = USaveGameHelper::GetSaveGame(this);
	if (IsValid(save_game))
	{
		_CurrentNickname = save_game->GetNickName();
		ShowSetNewNickname(_CurrentNickname.IsEmpty());
	}
}

bool UPage_InitUser::SetNewNickname(const FString& _new_nickname, FText& _out_error_text)
{
	auto save_game = USaveGameHelper::GetSaveGame_Editable(this);
	if (IsInvalid(save_game))
	{
		TRACE_ERROR(TEXT("Invalid Save Game"));
		return false;
	}

	FString prev_nickname = _CurrentNickname;

	bool success = save_game->SetNickName(_new_nickname, _out_error_text);

	if (success)
	{
		_CurrentNickname = save_game->GetNickName();
		ShowSetNewNickname(_CurrentNickname.IsEmpty());

		if(prev_nickname != _CurrentNickname)
		{
			USaveGameHelper::SaveGame(this);
		}
	}

	return success;
}
