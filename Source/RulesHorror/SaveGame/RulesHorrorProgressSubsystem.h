// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryFlowDefines.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "RulesHorrorProgressSubsystem.generated.h"

class URulesHorrorSaveGame;
class UStoryFlowSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDM_OnProgressFlagChanged, ERulesHorrorProgressFlag, _flag, bool, _value);

/*
 * 전역 진행 상태(낙인, 시나리오 해금/클리어, 엔딩 조건, 이어하기 위치)를 읽고 쓰는 곳.
 * - 값은 SaveGameSubsystem의 현재 SaveGame(URulesHorrorSaveGame)에 있다. 새 게임은 슬롯을 비우고 이어하기는 슬롯을 다시 읽으므로,
 *   여기서 읽는 값이 곧 복원된 상태다.
 * - StoryFlow 체크포인트 Shot(UStorySceneNodeData::_IsCheckpoint)에 들어가면 그 위치를 이어하기 위치로 저장한다.
 * - 활성 슬롯이 없으면(새 게임/이어하기 없이 바로 시작한 PIE, Shot부터 플레이) 메모리에만 반영하고 디스크에는 쓰지 않는다.
 */
UCLASS()
class RULESHORROR_API URulesHorrorProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UStoryFlowSubsystem> _StoryFlowSubsystem;

public:
	UPROPERTY(BlueprintAssignable, Category = "Progress|Event")
	FDM_OnProgressFlagChanged _OnProgressFlagChanged;

public:
	virtual void Initialize(FSubsystemCollectionBase& _collection) override;
	virtual void Deinitialize() override;

public:
	UFUNCTION(BlueprintPure, Category = "Progress")
	bool GetProgressFlag(ERulesHorrorProgressFlag _flag) const;

	/* 값이 바뀌면 _OnProgressFlagChanged를 보낸다. _write_to_disk면 활성 슬롯에 바로 저장한다. 값을 반영했으면 true. */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	bool SetProgressFlag(ERulesHorrorProgressFlag _flag, bool _value, bool _write_to_disk = true);

	/* 여러 플래그를 같은 값으로 바꾸고 한 번만 저장한다. 전부 반영했으면 true. */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	bool SetProgressFlags(const TArray<ERulesHorrorProgressFlag>& _flags, bool _value, bool _write_to_disk = true);

	UFUNCTION(BlueprintPure, Category = "Progress")
	bool AreAllProgressFlagsSet(const TArray<ERulesHorrorProgressFlag>& _flags) const;

	UFUNCTION(BlueprintPure, Category = "Progress")
	bool IsScenarioUnlocked(ERulesHorrorScenario _scenario) const;

	UFUNCTION(BlueprintPure, Category = "Progress")
	bool IsScenarioCleared(ERulesHorrorScenario _scenario) const;

	UFUNCTION(BlueprintPure, Category = "Progress")
	bool HasBrand(ERulesHorrorScenario _scenario) const;

	UFUNCTION(BlueprintPure, Category = "Progress")
	int32 GetBrandCount() const;

	UFUNCTION(BlueprintPure, Category = "Progress")
	bool IsEndingUnlocked() const;

	/* 마지막 체크포인트의 챕터 번호(CH02 -> 2). 아직 없으면 0. */
	UFUNCTION(BlueprintPure, Category = "Progress")
	int32 GetCurrentChapter() const;

	/* 이어하기 때 시작할 위치. */
	UFUNCTION(BlueprintPure, Category = "Progress")
	FStoryFlowRef GetCheckpointRef() const;

	/* 이어하기 위치를 기록하고 활성 슬롯에 저장한다. */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	bool SaveCheckpoint(const FStoryFlowRef& _checkpoint_ref);

	/* 현재 진행 상태를 활성 슬롯에 저장한다. 활성 슬롯이 없거나 저장에 실패하면 false. */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	bool WriteToDisk();

	/* Shot/Branch처럼 GameInstanceSubsystem 아래에 있는 오브젝트에서 찾을 때 쓴다. */
	static URulesHorrorProgressSubsystem* FindFromSubsystemOuter(const UObject* _object);

protected:
	URulesHorrorSaveGame* GetSaveGame() const;

	UFUNCTION()
	void OnStoryCheckpointReached(const FStoryFlowRef& _checkpoint_ref);
};
