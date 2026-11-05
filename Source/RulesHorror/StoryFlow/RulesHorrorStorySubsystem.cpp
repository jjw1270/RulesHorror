// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "RulesHorrorUtils.h"
#include "UI/Story/UI_StoryHUD.h"
#include "GameFramework/Pawn/InteractionPawn/InteractionPawn.h"
#include "GameFramework/Pawn/OfficePawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "SaveGameSubsystem.h"
#include "CustomSaveGame.h"

namespace
{
	static const float DialogueMinSkipSeconds = 0.35f;
}

bool URulesHorrorStorySubsystem::DoesSupportWorldType(const EWorldType::Type _world_type) const
{
	return _world_type == EWorldType::Game || _world_type == EWorldType::PIE;
}

void URulesHorrorStorySubsystem::Deinitialize()
{
	_OnDialogueFinished.Unbind();
	_DialogueLineIndex = INDEX_NONE;

	if (IsValid(_HUDWidget))
	{
		_HUDWidget->RemoveFromParent();
		_HUDWidget = nullptr;
	}

	Super::Deinitialize();
}

TStatId URulesHorrorStorySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URulesHorrorStorySubsystem, STATGROUP_Tickables);
}

URulesHorrorStorySubsystem* URulesHorrorStorySubsystem::Get(const UObject* _world_context)
{
	if (IsInvalid(_world_context) || GEngine == nullptr)
	{
		return nullptr;
	}

	UWorld* world = GEngine->GetWorldFromContextObject(_world_context, EGetWorldErrorMode::ReturnNull);
	return IsValid(world) ? world->GetSubsystem<URulesHorrorStorySubsystem>() : nullptr;
}

void URulesHorrorStorySubsystem::Tick(float _delta_time)
{
	Super::Tick(_delta_time);

	// 폰이 늦게 빙의되거나 바뀌어도 입력 잠금 상태를 맞춘다.
	if (_HasInputControl)
	{
		const APlayerController* pc = GetPlayerController();
		if (IsValid(pc) && _AppliedPawn.Get() != pc->GetPawn())
		{
			ApplyPlayerInputState();
		}
	}

	if (IsDialoguePlaying() == false)
	{
		return;
	}

	_DialogueLineElapsed += _delta_time;
	const bool is_skipped = _DialogueLineElapsed >= DialogueMinSkipSeconds && WasAdvanceKeyPressed();
	if (is_skipped || _DialogueLineElapsed >= _DialogueLineDuration)
	{
		ShowDialogueLine(_DialogueLineIndex + 1);
	}
}

#pragma region StoryEvent
bool URulesHorrorStorySubsystem::SendStoryEvent(FName _event_name)
{
	if (_event_name.IsNone())
	{
		return false;
	}

	const bool is_awaited = IsEventAwaited(_event_name);
	_ReceivedEvents.Add(_event_name);
	TRACE_LOG(TEXT("Story event: %s (awaited: %d)"), *_event_name.ToString(), is_awaited ? 1 : 0);

	_OnStoryEventNative.Broadcast(_event_name);
	_OnStoryEvent.Broadcast(_event_name);
	return is_awaited;
}

bool URulesHorrorStorySubsystem::Send(const UObject* _world_context, FName _event_name)
{
	URulesHorrorStorySubsystem* story_subsystem = Get(_world_context);
	return IsValid(story_subsystem) ? story_subsystem->SendStoryEvent(_event_name) : false;
}

void URulesHorrorStorySubsystem::ForgetStoryEvents(const TArray<FName>& _event_names)
{
	for (const FName& event_name : _event_names)
	{
		_ReceivedEvents.Remove(event_name);
	}
}

void URulesHorrorStorySubsystem::AddAwaitedEvents(const TArray<FName>& _event_names)
{
	for (const FName& event_name : _event_names)
	{
		_AwaitedEventCounts.FindOrAdd(event_name)++;
	}
}

void URulesHorrorStorySubsystem::RemoveAwaitedEvents(const TArray<FName>& _event_names)
{
	for (const FName& event_name : _event_names)
	{
		int32* count = _AwaitedEventCounts.Find(event_name);
		if (count == nullptr)
		{
			continue;
		}

		if (--(*count) <= 0)
		{
			_AwaitedEventCounts.Remove(event_name);
		}
	}
}

bool URulesHorrorStorySubsystem::IsEventAwaited(FName _event_name) const
{
	const int32* count = _AwaitedEventCounts.Find(_event_name);
	return count != nullptr && *count > 0;
}
#pragma endregion StoryEvent

#pragma region Dialogue
void URulesHorrorStorySubsystem::PlayDialogue(const TArray<FStoryDialogueLine>& _lines, const FOnStoryDialogueFinished& _on_finished)
{
	// 끝나기를 기다리던 대사를 덮으면 그쪽은 끝 알림을 못 받는다 (보통은 대사 중 조사를 막아서 생기지 않음).
	if (_OnDialogueFinished.IsBound())
	{
		TRACE_WARNING(TEXT("A dialogue waiting for its end was replaced. Its finish callback will not run."));
	}

	_OnDialogueFinished.Unbind();
	_DialogueLines = _lines;
	_OnDialogueFinished = _on_finished;
	ShowDialogueLine(0);
	_HasInputControl = true;
	ApplyPlayerInputState();
}

void URulesHorrorStorySubsystem::StopDialogue()
{
	_OnDialogueFinished.Unbind();
	FinishDialogue();
}

void URulesHorrorStorySubsystem::ShowDialogueLine(int32 _line_index)
{
	if (_DialogueLines.IsValidIndex(_line_index) == false)
	{
		FOnStoryDialogueFinished on_finished = _OnDialogueFinished;
		_OnDialogueFinished.Unbind();
		FinishDialogue();
		on_finished.ExecuteIfBound();
		return;
	}

	const FStoryDialogueLine& line = _DialogueLines[_line_index];
	const FText line_text = MakeLineText(line.Text);

	_DialogueLineIndex = _line_index;
	_DialogueLineElapsed = 0.0f;
	_DialogueLineDuration = line.Duration > 0.0f
		? line.Duration
		: FMath::Clamp(1.6f + 0.075f * line_text.ToString().Len(), 2.5f, 7.0f);

	UUI_StoryHUD* hud = GetOrCreateHUD();
	if (IsValid(hud))
	{
		hud->ShowDialogueLine(line.Speaker, line_text);
	}
}

void URulesHorrorStorySubsystem::FinishDialogue()
{
	const bool was_playing = IsDialoguePlaying();
	_DialogueLineIndex = INDEX_NONE;
	_DialogueLines.Reset();

	if (IsValid(_HUDWidget))
	{
		_HUDWidget->HideDialogue();
	}

	if (was_playing)
	{
		ApplyPlayerInputState();
	}
}

bool URulesHorrorStorySubsystem::WasAdvanceKeyPressed() const
{
	const APlayerController* pc = GetPlayerController();
	if (IsInvalid(pc))
	{
		return false;
	}

	return pc->WasInputKeyJustPressed(EKeys::LeftMouseButton)
		|| pc->WasInputKeyJustPressed(EKeys::SpaceBar)
		|| pc->WasInputKeyJustPressed(EKeys::Enter);
}

FText URulesHorrorStorySubsystem::MakeLineText(const FText& _text) const
{
	if (_text.ToString().Contains(TEXT("{Nickname}")) == false)
	{
		return _text;
	}

	FString nickname;
	if (const UCustomSaveGame* save_game = USaveGameHelper::GetCurrentSaveGame_ReadOnly(this))
	{
		save_game->FindSavedStringData(TEXT("Nickname"), nickname);
	}

	if (nickname.IsEmpty())
	{
		nickname = TEXT("손님");
	}

	FFormatNamedArguments args;
	args.Add(TEXT("Nickname"), FText::FromString(nickname));
	return FText::Format(_text, args);
}
#pragma endregion Dialogue

#pragma region Objective
void URulesHorrorStorySubsystem::SetObjective(const FText& _objective)
{
	UUI_StoryHUD* hud = GetOrCreateHUD();
	if (IsValid(hud))
	{
		hud->SetObjective(_objective);
	}
}
#pragma endregion Objective

#pragma region Input
void URulesHorrorStorySubsystem::SetPlayerInputLocked(bool _is_locked)
{
	_IsInputLocked = _is_locked;
	_HasInputControl = true;
	ApplyPlayerInputState();
}

void URulesHorrorStorySubsystem::ApplyPlayerInputState()
{
	APlayerController* pc = GetPlayerController();
	if (_HasInputControl == false || IsInvalid(pc))
	{
		return;
	}

	APawn* pawn = pc->GetPawn();
	if (pawn != _AppliedPawn.Get())
	{
		// 새 폰에 빙의하면 컨트롤러가 이동/시선 무시 횟수를 처음으로 되돌린다 (APlayerController::ClientRestart → ResetIgnoreInputFlags).
		if (IsValid(pawn))
		{
			_AppliedMoveLookLocked = false;
		}

		_AppliedPawn = pawn;
	}

	// 대사가 나오는 동안에도 조사 클릭이나 모니터 화면 클릭이 겹치지 않게 상호작용만 막는다.
	const bool is_interact_enabled = _IsInputLocked == false && IsDialoguePlaying() == false;
	if (AInteractionPawn* interaction_pawn = Cast<AInteractionPawn>(pawn))
	{
		interaction_pawn->SetInteractEnabled(is_interact_enabled);
		interaction_pawn->SetLookLocked(_IsInputLocked);
	}

	if (AOfficePawn* office_pawn = Cast<AOfficePawn>(pawn))
	{
		office_pawn->SetScreenInputEnabled(is_interact_enabled);
	}

	// SetIgnoreMoveInput/LookInput은 횟수로 쌓이므로 바뀔 때만 부른다.
	if (_AppliedMoveLookLocked != _IsInputLocked)
	{
		pc->SetIgnoreMoveInput(_IsInputLocked);
		pc->SetIgnoreLookInput(_IsInputLocked);
		_AppliedMoveLookLocked = _IsInputLocked;
	}
}
#pragma endregion Input

UUI_StoryHUD* URulesHorrorStorySubsystem::GetOrCreateHUD()
{
	if (IsValid(_HUDWidget))
	{
		return _HUDWidget;
	}

	APlayerController* pc = GetPlayerController();
	if (IsInvalid(pc))
	{
		return nullptr;
	}

	_HUDWidget = CreateWidget<UUI_StoryHUD>(pc, UUI_StoryHUD::StaticClass());
	if (IsValid(_HUDWidget))
	{
		_HUDWidget->AddToViewport((int32)ERulesHorrorWidgetZOrder::StoryHUD);
	}

	return _HUDWidget;
}

APlayerController* URulesHorrorStorySubsystem::GetPlayerController() const
{
	UWorld* world = GetWorld();
	return IsValid(world) ? world->GetFirstPlayerController() : nullptr;
}
