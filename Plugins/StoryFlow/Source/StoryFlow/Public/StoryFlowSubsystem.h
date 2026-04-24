// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "StoryFlowDefines.h"
#include "StoryFlowSubsystem.generated.h"

class UStorySceneAsset;
class UStorySceneBase;
class UStorySceneNodeData;
class UStoryShotBase;
class UWorld;

UENUM()
enum class EStoryFlowPendingTravelPhase : uint8
{
	None,
	LoadingLevel,
	TargetLevel,
};

UCLASS()
class STORYFLOW_API UStoryFlowSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TObjectPtr<UStorySceneAsset> _CurrentSceneAsset = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStorySceneNodeData> _CurrentShotNode = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStorySceneBase> _CurrentSceneInstance = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStoryShotBase> _CurrentShotInstance = nullptr;

	UPROPERTY(Transient)
	FStoryFlowRef _PendingStartRef;

	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> _PendingTargetLevel = nullptr;

	UPROPERTY(Transient)
	EStoryFlowPendingTravelPhase _PendingTravelPhase = EStoryFlowPendingTravelPhase::None;

public:
	virtual void Initialize(FSubsystemCollectionBase& _collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float _delta_time) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

public:
	UFUNCTION(BlueprintCallable)
	bool StartFromScene(const FStorySceneID& _scene_id);

	UFUNCTION(BlueprintCallable)
	bool StartFromRef(const FStoryFlowRef& _story_flow_ref);

	UFUNCTION(BlueprintCallable)
	void StopScene();

protected:
	UStorySceneAsset* FindSceneAssetBySceneID(const FStorySceneID& _scene_id) const;

	bool StartResolvedScene(UStorySceneAsset* _scene_asset, const FStoryFlowRef& _story_flow_ref);
	bool ShouldOpenTargetLevel(UStorySceneAsset* _scene_asset) const;
	void RequestOpenPendingLevel();
	void ClearPendingSceneStart();
	void HandlePostLoadMap(UWorld* _loaded_world);

	FString GetCurrentLevelPackageName() const;
	static FString GetLevelPackageName(const TSoftObjectPtr<UWorld>& _level);
	static FString GetWorldPackageName(const UWorld* _world);

	bool EnterScene(const FStoryFlowRef& _story_flow_ref);
	bool MoveToShot(const FStoryShotID& _shot_id);
	bool MoveToNextShot();

	void ClearCurrentScene();
	void ClearCurrentShot();

public:
	UFUNCTION(BlueprintPure, Category = "StoryFlow")
	UStorySceneAsset* GetCurrentSceneAsset() const { return _CurrentSceneAsset; }

	UFUNCTION(BlueprintPure, Category = "StoryFlow")
	UStoryShotBase* GetCurrentShot() const { return _CurrentShotInstance; }

	UFUNCTION(BlueprintPure, Category = "StoryFlow")
	UStorySceneBase* GetCurrentScene() const { return _CurrentSceneInstance; }

	UFUNCTION(BlueprintPure, Category = "StoryFlow")
	FStoryFlowRef GetCurrentRef() const;

};
