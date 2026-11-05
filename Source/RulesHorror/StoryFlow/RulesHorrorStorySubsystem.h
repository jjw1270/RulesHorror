// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StoryFlow/RulesHorrorStoryDefines.h"
#include "RulesHorrorStorySubsystem.generated.h"

class UUI_StoryHUD;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDM_OnStoryEvent, FName, _event_name);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStoryEventNative, FName);
DECLARE_DELEGATE(FOnStoryDialogueFinished);

/*
 * 게임 월드에서 StoryFlow Shot과 레벨 오브젝트를 이어 주는 서브시스템 (레벨마다 새로 생김).
 * - 스토리 이벤트: 조사 오브젝트/트리거/UI가 이름으로 보내고, Shot_WaitStoryEvent가 기다린다.
 * - 대사/자막, 목표 문구: 화면 아래 대사 상자와 왼쪽 위 목표 문구(UI_StoryHUD).
 * - 입력 잠금: 컷신 동안 이동·시선·상호작용을 막는다.
 */
UCLASS()
class RULESHORROR_API URulesHorrorStorySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TObjectPtr<UUI_StoryHUD> _HUDWidget = nullptr;

	TSet<FName> _ReceivedEvents;
	TMap<FName, int32> _AwaitedEventCounts;

	TArray<FStoryDialogueLine> _DialogueLines;
	int32 _DialogueLineIndex = INDEX_NONE;
	float _DialogueLineElapsed = 0.0f;
	float _DialogueLineDuration = 0.0f;
	FOnStoryDialogueFinished _OnDialogueFinished;

	bool _IsInputLocked = false;
	/* 스토리 Shot이 입력 잠금이나 대사를 한 번이라도 쓴 뒤에만 폰 입력 상태를 건드린다 (로비 인트로 등 다른 흐름을 덮어쓰지 않게). */
	bool _HasInputControl = false;
	bool _AppliedMoveLookLocked = false;
	TWeakObjectPtr<APawn> _AppliedPawn;

public:
	UPROPERTY(BlueprintAssignable, Category = "Story")
	FDM_OnStoryEvent _OnStoryEvent;

	FOnStoryEventNative _OnStoryEventNative;

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type _world_type) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float _delta_time) override;
	virtual TStatId GetStatId() const override;

	static URulesHorrorStorySubsystem* Get(const UObject* _world_context);

#pragma region StoryEvent
public:
	/* 이벤트를 보낸다. 지금 기다리는 Shot이 있으면 true (보낸 쪽이 원래 동작을 대신 맡길 때 쓴다). */
	UFUNCTION(BlueprintCallable, Category = "Story")
	bool SendStoryEvent(FName _event_name);

	/* 이 레벨에서 이미 받은 이벤트인지 */
	UFUNCTION(BlueprintPure, Category = "Story")
	bool HasReceivedStoryEvent(FName _event_name) const { return _ReceivedEvents.Contains(_event_name); }

	void ForgetStoryEvents(const TArray<FName>& _event_names);
	void AddAwaitedEvents(const TArray<FName>& _event_names);
	void RemoveAwaitedEvents(const TArray<FName>& _event_names);
	bool IsEventAwaited(FName _event_name) const;

	/* 월드 컨텍스트로 서브시스템을 찾아 이벤트를 보낸다. 서브시스템이 없으면 false. */
	static bool Send(const UObject* _world_context, FName _event_name);
#pragma endregion StoryEvent

#pragma region Dialogue
public:
	/* 대사를 처음부터 보여준다. 이미 나오던 대사는 끊는다(끊긴 쪽 콜백은 부르지 않음). */
	void PlayDialogue(const TArray<FStoryDialogueLine>& _lines, const FOnStoryDialogueFinished& _on_finished);
	void StopDialogue();

	UFUNCTION(BlueprintPure, Category = "Story")
	bool IsDialoguePlaying() const { return _DialogueLineIndex != INDEX_NONE; }

protected:
	void ShowDialogueLine(int32 _line_index);
	void FinishDialogue();
	bool WasAdvanceKeyPressed() const;
	FText MakeLineText(const FText& _text) const;
#pragma endregion Dialogue

#pragma region Objective
public:
	/* 왼쪽 위 목표 문구. 빈 문구면 숨긴다. */
	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetObjective(const FText& _objective);
#pragma endregion Objective

#pragma region Input
public:
	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetPlayerInputLocked(bool _is_locked);

	UFUNCTION(BlueprintPure, Category = "Story")
	bool IsPlayerInputLocked() const { return _IsInputLocked; }

protected:
	void ApplyPlayerInputState();
#pragma endregion Input

protected:
	UUI_StoryHUD* GetOrCreateHUD();
	APlayerController* GetPlayerController() const;
};
