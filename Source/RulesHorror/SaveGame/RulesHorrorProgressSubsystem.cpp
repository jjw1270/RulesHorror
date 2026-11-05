// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGame/RulesHorrorProgressSubsystem.h"
#include "SaveGame/RulesHorrorSaveGame.h"
#include "SaveGameSubsystem.h"
#include "StoryFlowSubsystem.h"
#include "RulesHorrorUtils.h"
#include "Engine/GameInstance.h"

void URulesHorrorProgressSubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

	_collection.InitializeDependency<USaveGameSubsystem>();

	UStoryFlowSubsystem* story_flow_subsystem = _collection.InitializeDependency<UStoryFlowSubsystem>();
	if (IsValid(story_flow_subsystem))
	{
		_StoryFlowSubsystem = story_flow_subsystem;
		story_flow_subsystem->_OnCheckpointReached.AddDynamic(this, &URulesHorrorProgressSubsystem::OnStoryCheckpointReached);
	}
	else
	{
		TRACE_ERROR(TEXT("story flow subsystem invalid."));
	}
}

void URulesHorrorProgressSubsystem::Deinitialize()
{
	if (_StoryFlowSubsystem.IsValid())
	{
		_StoryFlowSubsystem->_OnCheckpointReached.RemoveDynamic(this, &URulesHorrorProgressSubsystem::OnStoryCheckpointReached);
	}
	_StoryFlowSubsystem.Reset();

	Super::Deinitialize();
}

URulesHorrorSaveGame* URulesHorrorProgressSubsystem::GetSaveGame() const
{
	const UGameInstance* game_instance = GetGameInstance();
	const USaveGameSubsystem* save_game_subsystem = IsValid(game_instance) ? game_instance->GetSubsystem<USaveGameSubsystem>() : nullptr;
	return IsValid(save_game_subsystem) ? Cast<URulesHorrorSaveGame>(save_game_subsystem->GetCurrentSaveGame()) : nullptr;
}

URulesHorrorProgressSubsystem* URulesHorrorProgressSubsystem::FindFromSubsystemOuter(const UObject* _object)
{
	for (const UObject* outer = _object; IsValid(outer); outer = outer->GetOuter())
	{
		if (const UGameInstanceSubsystem* game_instance_subsystem = Cast<UGameInstanceSubsystem>(outer))
		{
			const UGameInstance* game_instance = game_instance_subsystem->GetGameInstance();
			return IsValid(game_instance) ? game_instance->GetSubsystem<URulesHorrorProgressSubsystem>() : nullptr;
		}

		if (const UGameInstance* game_instance = Cast<UGameInstance>(outer))
		{
			return game_instance->GetSubsystem<URulesHorrorProgressSubsystem>();
		}
	}

	return URulesHorrorUtils::GetGameInstanceSubsystem<URulesHorrorProgressSubsystem>(_object);
}

bool URulesHorrorProgressSubsystem::GetProgressFlag(ERulesHorrorProgressFlag _flag) const
{
	const URulesHorrorSaveGame* save_game = GetSaveGame();
	return IsValid(save_game) && save_game->GetProgressFlag(_flag);
}

bool URulesHorrorProgressSubsystem::SetProgressFlag(ERulesHorrorProgressFlag _flag, bool _value, bool _write_to_disk)
{
	if (RulesHorrorProgress::IsValidFlag(_flag) == false)
	{
		TRACE_WARNING(TEXT("invalid progress flag : %d"), static_cast<int32>(_flag));
		return false;
	}

	URulesHorrorSaveGame* save_game = GetSaveGame();
	if (IsInvalid(save_game))
	{
		TRACE_ERROR(TEXT("save game invalid."));
		return false;
	}

	const bool previous_value = save_game->GetProgressFlag(_flag);
	if (save_game->SaveProgressFlag(_flag, _value) == false)
	{
		return false;
	}

	if (previous_value != _value)
	{
		CUSTOM_LOG(Display, TEXT("Progress flag %s = %s"), *TEnumToString(_flag), _value ? TEXT("true") : TEXT("false"));
		_OnProgressFlagChanged.Broadcast(_flag, _value);
	}

	if (_write_to_disk)
	{
		WriteToDisk();
	}

	return true;
}

bool URulesHorrorProgressSubsystem::SetProgressFlags(const TArray<ERulesHorrorProgressFlag>& _flags, bool _value, bool _write_to_disk)
{
	bool all_applied = true;
	for (const ERulesHorrorProgressFlag flag : _flags)
	{
		all_applied &= SetProgressFlag(flag, _value, false);
	}

	if (_write_to_disk && _flags.Num() > 0)
	{
		WriteToDisk();
	}

	return all_applied;
}

bool URulesHorrorProgressSubsystem::AreAllProgressFlagsSet(const TArray<ERulesHorrorProgressFlag>& _flags) const
{
	for (const ERulesHorrorProgressFlag flag : _flags)
	{
		if (GetProgressFlag(flag) == false)
			return false;
	}

	return true;
}

bool URulesHorrorProgressSubsystem::IsScenarioUnlocked(ERulesHorrorScenario _scenario) const
{
	return GetProgressFlag(RulesHorrorProgress::GetUnlockedFlag(_scenario));
}

bool URulesHorrorProgressSubsystem::IsScenarioCleared(ERulesHorrorScenario _scenario) const
{
	return GetProgressFlag(RulesHorrorProgress::GetClearFlag(_scenario));
}

bool URulesHorrorProgressSubsystem::HasBrand(ERulesHorrorScenario _scenario) const
{
	return GetProgressFlag(RulesHorrorProgress::GetBrandFlag(_scenario));
}

int32 URulesHorrorProgressSubsystem::GetBrandCount() const
{
	int32 brand_count = 0;
	for (int32 i = 1; i <= RulesHorrorProgress::ScenarioCount; ++i)
	{
		brand_count += HasBrand(static_cast<ERulesHorrorScenario>(i)) ? 1 : 0;
	}

	return brand_count;
}

bool URulesHorrorProgressSubsystem::IsEndingUnlocked() const
{
	return GetProgressFlag(ERulesHorrorProgressFlag::EndingUnlocked);
}

int32 URulesHorrorProgressSubsystem::GetCurrentChapter() const
{
	const URulesHorrorSaveGame* save_game = GetSaveGame();
	return IsValid(save_game) ? save_game->GetCurrentChapter() : 0;
}

FStoryFlowRef URulesHorrorProgressSubsystem::GetCheckpointRef() const
{
	const URulesHorrorSaveGame* save_game = GetSaveGame();
	return IsValid(save_game) ? save_game->GetStoryFlowRef() : FStoryFlowRef();
}

bool URulesHorrorProgressSubsystem::SaveCheckpoint(const FStoryFlowRef& _checkpoint_ref)
{
	if (_checkpoint_ref.IsValid() == false)
	{
		TRACE_WARNING(TEXT("checkpoint ref invalid."));
		return false;
	}

	URulesHorrorSaveGame* save_game = GetSaveGame();
	if (IsInvalid(save_game))
	{
		TRACE_ERROR(TEXT("save game invalid."));
		return false;
	}

	save_game->SaveStoryFlowRef(_checkpoint_ref);
	CUSTOM_LOG(Display, TEXT("Checkpoint %s / %s (chapter %d)"),
		*_checkpoint_ref.SceneID.Get().ToString(), *_checkpoint_ref.ShotID.Get().ToString(), save_game->GetCurrentChapter());

	return WriteToDisk();
}

bool URulesHorrorProgressSubsystem::WriteToDisk()
{
	const UGameInstance* game_instance = GetGameInstance();
	USaveGameSubsystem* save_game_subsystem = IsValid(game_instance) ? game_instance->GetSubsystem<USaveGameSubsystem>() : nullptr;
	if (IsInvalid(save_game_subsystem))
	{
		TRACE_ERROR(TEXT("save game subsystem invalid."));
		return false;
	}

	const int32 slot_index = save_game_subsystem->GetActiveSaveGameSlotIndex();
	if (slot_index == INDEX_NONE)
	{
		CUSTOM_LOG(Display, TEXT("No active save slot (not started from New Game/Continue). Progress kept in memory only."));
		return false;
	}

	const bool save_success = save_game_subsystem->SaveGameSlot(slot_index);
	if (save_success == false)
	{
		TRACE_WARNING(TEXT("progress save failed. slot : %d"), slot_index);
	}

	return save_success;
}

void URulesHorrorProgressSubsystem::OnStoryCheckpointReached(const FStoryFlowRef& _checkpoint_ref)
{
	SaveCheckpoint(_checkpoint_ref);
}
