// Copyright (c) 2026 장윤제. All rights reserved.


#include "RulesHorrorGameInstance.h"
#include "RulesHorrorUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "RulesHorrorDeveloperSettings.h"
#include "SaveGameSubsystem.h"
#include "SaveGame/RulesHorrorSaveGame.h"
#include "StoryFlowSubsystem.h"
#include "UI/Office/UI_Monitor.h"

namespace
{
	/* 모니터 화면이 기억하는 마지막 상태(정적 변수)를 지운다. 레벨이 바뀌어도 남으므로 새 게임/이어하기/종료 때 부른다.
	   모니터 첫 화면은 다음에 켤 때 저장(사이트 가입 여부)을 보고 다시 정한다. */
	void ResetMonitorScreenState()
	{
		UUI_Monitor::ResetLastActiveWidgetindex();
	}
}

void URulesHorrorGameInstance::Shutdown()
{
	ResetMonitorScreenState();

	Super::Shutdown();
}

void URulesHorrorGameInstance::QuitGame()
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsInvalid(pc))
		return;

	UKismetSystemLibrary::QuitGame(this, pc, EQuitPreference::Quit, false);
}

void URulesHorrorGameInstance::PauseGame(bool _is_pause)
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsInvalid(pc))
		return;	

	pc->SetPause(_is_pause);

	// UGameplayStatics::SetGlobalTimeDilation(GetWorld(), _is_pause ? 0.0f : 1.0f);
}

bool URulesHorrorGameInstance::CanStartSavedGame() const
{
	const auto save_game = Cast<URulesHorrorSaveGame>(USaveGameHelper::GetSaveGameFromSlot_ReadOnly(this, SaveGameSlotIndex));
	if (IsValid(save_game))
	{
		const auto& saved_story_flow_ref = save_game->GetStoryFlowRef();
		return saved_story_flow_ref.IsValid();
	}

	return false;
}

void URulesHorrorGameInstance::StartNewGame()
{
	auto save_game_subsystem = GetSubsystem<USaveGameSubsystem>();
	if (IsInvalid(save_game_subsystem))
	{
		TRACE_ERROR(TEXT("save game subsystem invalid."));
		return;
	}

	auto story_flow_subsystem = GetSubsystem<UStoryFlowSubsystem>();
	if (IsInvalid(story_flow_subsystem))
	{
		TRACE_ERROR(TEXT("story flow subsystem invalid."));
		return;
	}

	const bool save_game_slot_ready = save_game_subsystem->IsValidSaveGameSlotIndex(SaveGameSlotIndex)
		? save_game_subsystem->ResetGameSlot(SaveGameSlotIndex)
		: save_game_subsystem->CreateNewGameSlot(SaveGameSlotIndex);
	if (save_game_slot_ready == false)
	{
		TRACE_ERROR(TEXT("ready save game slot failed."));
		return;
	}

	auto dev_setting = GetDefault<URulesHorrorDeveloperSettings>();
	if (IsInvalid(dev_setting))
	{
		TRACE_ERROR(TEXT("Rules Horror dev setting invalid."));
		return;
	}

	if (dev_setting->_StoryStartSceneID.IsValid() == false)
	{
		TRACE_ERROR(TEXT("Rules Horror dev setting _StoryStartSceneID is empty."));
		return;
	}

	auto save_game = Cast<URulesHorrorSaveGame>(save_game_subsystem->GetCurrentSaveGame());
	if (IsInvalid(save_game))
	{
		TRACE_ERROR(TEXT("save game invalid."));
		return;
	}

	// 시작 Scene이 체크포인트를 저장하기 전에 끝나도 이어하기가 되도록 시작 위치를 먼저 저장한다.
	FStoryFlowRef start_ref;
	start_ref.SceneID = dev_setting->_StoryStartSceneID;
	save_game->SaveStoryFlowRef(start_ref);
	save_game_subsystem->SaveGameSlot(SaveGameSlotIndex);

	// 모니터는 레벨이 바뀌어도 마지막 화면을 기억하므로, 새로 시작할 때는 부팅·닉네임(사이트 가입) 화면부터 다시 보이게 한다.
	ResetMonitorScreenState();
	story_flow_subsystem->StartFromRef(start_ref);
}

void URulesHorrorGameInstance::StartSavedGame()
{
	auto save_game_subsystem = GetSubsystem<USaveGameSubsystem>();
	if (IsInvalid(save_game_subsystem))
	{
		TRACE_ERROR(TEXT("save game subsystem invalid."));
		return;
	}

	if (save_game_subsystem->LoadGameSlot(SaveGameSlotIndex) == false)
	{
		TRACE_ERROR(TEXT("load save game slot failed."));
		return;
	}

	const auto save_game = Cast<URulesHorrorSaveGame>(save_game_subsystem->GetCurrentSaveGame());
	if (IsInvalid(save_game))
	{
		TRACE_ERROR(TEXT("save game invalid."));
		return;
	}

	auto story_flow_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<UStoryFlowSubsystem>(this);
	if (IsInvalid(story_flow_subsystem))
	{
		TRACE_ERROR(TEXT("story flow subsystem invalid."));
		return;
	}

	// 이어하기: 이미 가입했으면 모니터가 바탕화면부터 보인다 (UUI_Monitor::GetStartWidgetIndex).
	ResetMonitorScreenState();
	story_flow_subsystem->StartFromRef(save_game->GetStoryFlowRef());
}
