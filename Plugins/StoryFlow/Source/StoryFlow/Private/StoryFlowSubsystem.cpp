// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlowSubsystem.h"
#include "StorySceneAsset.h"
#include "StoryBranchBase.h"
#include "StoryBranchNodeData.h"
#include "StorySceneBase.h"
#include "StorySceneNodeData.h"
#include "StoryFlowDeveloperSettings.h"
#include "StorySceneRegistryAsset.h"
#include "StoryShotBase.h"
#include "Curves/CurveFloat.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Subsystems/SubsystemCollection.h"
#include "UObject/UObjectGlobals.h"
#include "CommonUtils.h"

void UStoryFlowSubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UStoryFlowSubsystem::HandlePostLoadMap);
}

void UStoryFlowSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	StopScene();
	ClearPendingSceneStart();

	Super::Deinitialize();
}

const UStoryFlowDeveloperSettings* UStoryFlowSubsystem::GetStoryFlowDeveloperSettings() const
{
	return GetDefault<UStoryFlowDeveloperSettings>();
}

TStatId UStoryFlowSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStoryFlowSubsystem, STATGROUP_Tickables);
}

bool UStoryFlowSubsystem::IsTickable() const
{
	return IsValid(_CurrentShotInstance) || _PendingTravelPhase == EStoryFlowPendingTravelPhase::AsyncLoadingTargetLevel;
}

void UStoryFlowSubsystem::Tick(float _delta_time)
{
	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::AsyncLoadingTargetLevel)
	{
		if (_PendingTargetLevelLoadCompleted && GetMinimumLoadingTimeProgress() >= 1.0f)
		{
			_PendingTravelPhase = EStoryFlowPendingTravelPhase::OpeningTargetLevel;
			CUSTOM_LOG(Display, TEXT("Async load + minimum duration satisfied. Open TargetLevel=%s"), *GetLevelPackageName(_PendingTargetLevel));
			UGameplayStatics::OpenLevelBySoftObjectPtr(this, _PendingTargetLevel);
		}
	}

	if (IsInvalid(_CurrentShotInstance))
		return;

	_CurrentShotInstance->TickShot(_delta_time);
	if (_CurrentShotInstance->IsFinished())
	{
		if (MoveToNextShot() == false)
		{
			StopScene();
		}
	}
}

bool UStoryFlowSubsystem::StartFromScene(const FStorySceneID& _scene_id)
{
	if (_scene_id.IsValid() == false)
		return false;

	FStoryFlowRef story_flow_ref;
	story_flow_ref.SceneID = _scene_id;

	return StartFromRef(story_flow_ref);
}

bool UStoryFlowSubsystem::StartFromRef(const FStoryFlowRef& _story_flow_ref)
{
	UStorySceneAsset* scene_asset = FindSceneAssetBySceneID(_story_flow_ref.SceneID);

	if (IsInvalid(scene_asset))
		return false;

	if (ShouldOpenTargetLevel(scene_asset))
	{
		return BeginPendingSceneTravel(scene_asset, _story_flow_ref);
	}

	return StartResolvedScene(scene_asset, _story_flow_ref);
}

void UStoryFlowSubsystem::StopScene()
{
	ClearCurrentShot();
	ClearCurrentBranch();
	ClearCurrentScene();
	_CurrentSceneAsset = nullptr;
}

UStorySceneAsset* UStoryFlowSubsystem::FindSceneAssetBySceneID(const FStorySceneID& _scene_id) const
{
	if (_scene_id.IsValid() == false)
		return nullptr;

	const UStoryFlowDeveloperSettings* dev_settings = GetStoryFlowDeveloperSettings();
	if (IsInvalid(dev_settings) || dev_settings->_StorySceneRegistry.IsNull())
		return nullptr;

	UStorySceneRegistryAsset* scene_registry = dev_settings->_StorySceneRegistry.LoadSynchronous();
	if (IsInvalid(scene_registry))
		return nullptr;

	return scene_registry->FindSceneAsset(_scene_id);
}

FStoryFlowRef UStoryFlowSubsystem::MakeResolvedStartRef(UStorySceneAsset* _scene_asset, const FStoryFlowRef& _story_flow_ref) const
{
	FStoryFlowRef start_ref = _story_flow_ref;
	if (IsValid(_scene_asset))
	{
		start_ref.SceneID = _scene_asset->GetSceneID();
		start_ref.ShotID = start_ref.ShotID.IsValid() ? start_ref.ShotID : _scene_asset->GetEntryShotID();
	}

	return start_ref;
}

bool UStoryFlowSubsystem::BeginPendingSceneTravel(UStorySceneAsset* _scene_asset, const FStoryFlowRef& _story_flow_ref)
{
	if (IsInvalid(_scene_asset))
	{
		return false;
	}

	StopScene();

	_PendingStartRef = MakeResolvedStartRef(_scene_asset, _story_flow_ref);
	_PendingTargetLevel = _scene_asset->GetTargetLevel();

	const UStoryFlowDeveloperSettings* dev_settings = GetStoryFlowDeveloperSettings();
	if (IsInvalid(dev_settings) || dev_settings->_LoadingLevel.IsNull())
	{
		TRACE_ERROR(TEXT("BeginPendingSceneTravel failed: LoadingLevel is required but missing."));
		ClearPendingSceneStart();
		return false;
	}

	if (GetLevelPackageName(dev_settings->_LoadingLevel) == GetLevelPackageName(_PendingTargetLevel))
	{
		TRACE_ERROR(TEXT("BeginPendingSceneTravel failed: LoadingLevel must be different from TargetLevel."));
		ClearPendingSceneStart();
		return false;
	}

	_PendingTravelPhase = EStoryFlowPendingTravelPhase::LoadingLevel;
	RequestOpenLoadingLevel();
	return true;
}

bool UStoryFlowSubsystem::StartResolvedScene(UStorySceneAsset* _scene_asset, const FStoryFlowRef& _story_flow_ref)
{
	if (IsInvalid(_scene_asset))
		return false;

	StopScene();
	_CurrentSceneAsset = _scene_asset;

	const FStoryFlowRef start_ref = MakeResolvedStartRef(_CurrentSceneAsset, _story_flow_ref);

	if (EnterScene(start_ref) == false)
	{
		StopScene();
		return false;
	}

	if (MoveToShot(start_ref.ShotID) == false)
	{
		StopScene();
		return false;
	}

	return true;
}

bool UStoryFlowSubsystem::ShouldOpenTargetLevel(UStorySceneAsset* _scene_asset) const
{
	if (IsInvalid(_scene_asset) || _scene_asset->GetTargetLevel().IsNull())
		return false;

	return GetCurrentLevelPackageName() != GetLevelPackageName(_scene_asset->GetTargetLevel());
}

bool UStoryFlowSubsystem::BeginAsyncLoadTargetLevel()
{
	if (_PendingTargetLevel.IsNull())
	{
		TRACE_WARNING(TEXT("BeginAsyncLoadTargetLevel failed: target level missing."));
		return false;
	}

	const FSoftObjectPath target_level_path = _PendingTargetLevel.ToSoftObjectPath();
	if (target_level_path.IsValid() == false)
	{
		TRACE_WARNING(TEXT("BeginAsyncLoadTargetLevel failed: target level path invalid."));
		return false;
	}

	_PendingTargetLevelLoadHandle.Reset();
	_PendingTravelPhase = EStoryFlowPendingTravelPhase::AsyncLoadingTargetLevel;
	_PendingTargetLevelLoadCompleted = false;
	FStreamableDelegate on_loaded = FStreamableDelegate::CreateUObject(this, &UStoryFlowSubsystem::HandleTargetLevelAsyncLoaded);
	_PendingTargetLevelLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(target_level_path, MoveTemp(on_loaded));
	if (IsInvalid(_PendingTargetLevelLoadHandle))
	{
		TRACE_WARNING(TEXT("Async load request failed immediately. TargetLevel=%s"), *target_level_path.ToString());
		_PendingTravelPhase = EStoryFlowPendingTravelPhase::None;
		return false;
	}

	return true;
}

void UStoryFlowSubsystem::RequestOpenLoadingLevel()
{
	const UStoryFlowDeveloperSettings* dev_settings = GetStoryFlowDeveloperSettings();
	check(IsValid(dev_settings));
	check(dev_settings->_LoadingLevel.IsNull() == false);

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, dev_settings->_LoadingLevel);
}

void UStoryFlowSubsystem::HandleTargetLevelAsyncLoaded()
{
	if (_PendingTravelPhase != EStoryFlowPendingTravelPhase::AsyncLoadingTargetLevel)
		return;

	if (_PendingTargetLevel.IsNull())
	{
		TRACE_WARNING(TEXT("Async load callback failed: pending target level missing."));
		ClearPendingSceneStart();
		return;
	}

	_PendingTargetLevelLoadCompleted = true;
}

void UStoryFlowSubsystem::ClearPendingSceneStart()
{
	if (_PendingTargetLevelLoadHandle.IsValid())
	{
		_PendingTargetLevelLoadHandle->CancelHandle();
		_PendingTargetLevelLoadHandle.Reset();
	}

	_PendingStartRef.Reset();
	_PendingTargetLevel = nullptr;
	_PendingTravelPhase = EStoryFlowPendingTravelPhase::None;
	_PendingTargetLevelLoadCompleted = false;
	_PendingLoadingLevelEnterTime = 0.0;
}

void UStoryFlowSubsystem::HandlePostLoadMap(UWorld* _loaded_world)
{
	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::None || IsInvalid(_loaded_world))
	{
		return;
	}

	const UStoryFlowDeveloperSettings* dev_settings = GetStoryFlowDeveloperSettings();
	const FString loaded_world_package_name = GetWorldPackageName(_loaded_world);

	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::LoadingLevel)
	{
		const FString loading_level_package_name = IsValid(dev_settings) ? GetLevelPackageName(dev_settings->_LoadingLevel) : FString();
		if (loading_level_package_name.IsEmpty() || loaded_world_package_name != loading_level_package_name)
		{
			return;
		}

		_PendingLoadingLevelEnterTime = FPlatformTime::Seconds();
		if (BeginAsyncLoadTargetLevel() == false)
		{
			ClearPendingSceneStart();
		}
		return;
	}

	if (_PendingTravelPhase != EStoryFlowPendingTravelPhase::OpeningTargetLevel)
	{
		return;
	}

	if (loaded_world_package_name != GetLevelPackageName(_PendingTargetLevel))
	{
		return;
	}

	const FStoryFlowRef pending_start_ref = _PendingStartRef;
	ClearPendingSceneStart();

	UStorySceneAsset* scene_asset = FindSceneAssetBySceneID(pending_start_ref.SceneID);
	if (IsInvalid(scene_asset))
	{
		return;
	}

	StartResolvedScene(scene_asset, pending_start_ref);
}

FString UStoryFlowSubsystem::GetCurrentLevelPackageName() const
{
	const UGameInstance* game_instance = GetGameInstance();
	const UWorld* world = IsValid(game_instance) ? game_instance->GetWorld() : nullptr;
	return GetWorldPackageName(world);
}

FString UStoryFlowSubsystem::GetLevelPackageName(const TSoftObjectPtr<UWorld>& _level)
{
	if (_level.IsNull())
	{
		return FString();
	}

	return FPackageName::ObjectPathToPackageName(_level.ToSoftObjectPath().ToString());
}

FString UStoryFlowSubsystem::GetWorldPackageName(const UWorld* _world)
{
	if (IsInvalid(_world))
	{
		return FString();
	}

	const ULevel* current_level = _world->GetCurrentLevel();
	if (IsValid(current_level))
	{
		return UWorld::RemovePIEPrefix(current_level->GetOutermost()->GetName());
	}

	return UWorld::RemovePIEPrefix(_world->GetOutermost()->GetName());
}

float UStoryFlowSubsystem::GetMinimumLoadingTimeProgress() const
{
	if (_PendingLoadingLevelEnterTime <= 0.0)
	{
		return 1.0f;
	}

	const UStoryFlowDeveloperSettings* dev_settings = GetStoryFlowDeveloperSettings();
	const float minimum_loading_duration = IsValid(dev_settings) ? dev_settings->_MinimumLoadingLevelDuration : 0.0f;
	if (minimum_loading_duration <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const double elapsed_time = FPlatformTime::Seconds() - _PendingLoadingLevelEnterTime;
	const float normalized_time_progress = FMath::Clamp(static_cast<float>(elapsed_time / minimum_loading_duration), 0.0f, 1.0f);

	if (IsValid(dev_settings) && dev_settings->_MinimumLoadingLevelProgressCurve.IsNull() == false)
	{
		if (UCurveFloat* progress_curve = dev_settings->_MinimumLoadingLevelProgressCurve.LoadSynchronous())
		{
			return FMath::Clamp(progress_curve->GetFloatValue(normalized_time_progress), 0.0f, 1.0f);
		}
	}

	return normalized_time_progress;
}

bool UStoryFlowSubsystem::EnterScene(const FStoryFlowRef& _story_flow_ref)
{
	if (IsInvalid(_CurrentSceneAsset))
	{
		return false;
	}

	UStorySceneBase* scene_template = _CurrentSceneAsset->GetSceneTemplate();
	if (IsInvalid(scene_template))
	{
		return true;
	}

	_CurrentSceneInstance = DuplicateObject<UStorySceneBase>(scene_template, this);
	if (IsInvalid(_CurrentSceneInstance))
	{
		return false;
	}

	_CurrentSceneInstance->InitializeScene(_story_flow_ref);
	_CurrentSceneInstance->EnterScene();
	return true;
}

bool UStoryFlowSubsystem::MoveToShot(const FStoryShotID& _shot_id)
{
	if (IsInvalid(_CurrentSceneAsset) || _shot_id.IsValid() == false)
	{
		return false;
	}

	UStorySceneNodeData* shot_node = _CurrentSceneAsset->FindShotNode(_shot_id);
	if (IsInvalid(shot_node))
	{
		return false;
	}

	UStoryShotBase* shot_template = shot_node->GetShotTemplate();
	if (IsInvalid(shot_template))
	{
		return false;
	}

	ClearCurrentShot();

	_CurrentShotNode = shot_node;
	_CurrentShotInstance = DuplicateObject<UStoryShotBase>(shot_template, this);
	if (IsInvalid(_CurrentShotInstance))
	{
		_CurrentShotNode = nullptr;
		return false;
	}

	_CurrentShotInstance->InitializeShot(_shot_id);
	_CurrentShotInstance->EnterShot();
	return true;
}

bool UStoryFlowSubsystem::EvaluateBranch(const FStoryBranchID& _branch_id, const FStoryFlowRef& _story_flow_ref)
{
	if (IsInvalid(_CurrentSceneAsset) || _branch_id.IsValid() == false)
	{
		return false;
	}

	UStoryBranchNodeData* branch_node = _CurrentSceneAsset->FindBranchNode(_branch_id);
	if (IsInvalid(branch_node))
	{
		return false;
	}

	UStoryBranchBase* branch_template = branch_node->GetBranchTemplate();
	if (IsInvalid(branch_template))
	{
		return false;
	}

	_CurrentBranchNode = branch_node;
	UStoryBranchBase* branch_instance = DuplicateObject<UStoryBranchBase>(branch_template, this);
	if (IsInvalid(branch_instance))
	{
		_CurrentBranchNode = nullptr;
		return false;
	}

	branch_instance->InitializeBranch(_story_flow_ref);

	const TMap<int32, FStorySceneBranchLink>& next_links_by_pin_index = branch_node->GetNextLinksByPinIndex();
	if (next_links_by_pin_index.Num() == 0)
	{
		_CurrentBranchNode = nullptr;
		return false;
	}

	int32 next_index = 0;
	if (branch_node->GetBranchCount() > 1)
	{
		next_index = branch_instance->SelectNextIndex(branch_node->GetBranchCount());
	}

	next_index = FMath::Clamp(next_index, 0, FMath::Max(branch_node->GetBranchCount() - 1, 0));
	const FStorySceneBranchLink* next_link = next_links_by_pin_index.Find(next_index);
	_CurrentBranchNode = nullptr;

	if (next_link == nullptr || next_link->IsValid() == false)
	{
		return false;
	}

	if (next_link->IsShotLink())
	{
		return MoveToShot(next_link->NextShotID);
	}

	if (next_link->IsSceneLink())
	{
		return StartFromScene(next_link->NextSceneID);
	}

	return false;
}

bool UStoryFlowSubsystem::MoveToNextShot()
{
	if (IsInvalid(_CurrentShotNode))
	{
		return false;
	}

	UStoryShotBase* current_shot = _CurrentShotInstance;
	if (IsValid(current_shot))
	{
		current_shot->ExitShot();
	}

	const TArray<FStorySceneBranchLink>& next_links = _CurrentShotNode->GetNextLinks();
	if (next_links.Num() == 0)
	{
		_CurrentShotInstance = nullptr;
		_CurrentShotNode = nullptr;
		return false;
	}

	int32 next_index = 0;
	if (next_links.Num() > 1 && IsValid(current_shot))
	{
		next_index = current_shot->SelectNextShotIndex(next_links.Num());
	}

	next_index = FMath::Clamp(next_index, 0, next_links.Num() - 1);
	const FStorySceneBranchLink& next_link = next_links[next_index];

	_CurrentShotInstance = nullptr;
	_CurrentShotNode = nullptr;

	if (next_link.IsShotLink())
	{
		return MoveToShot(next_link.NextShotID);
	}

	if (next_link.IsBranchLink())
	{
		return EvaluateBranch(next_link.NextBranchID, GetCurrentRef());
	}

	if (next_link.IsSceneLink())
	{
		return StartFromScene(next_link.NextSceneID);
	}

	return false;
}

void UStoryFlowSubsystem::ClearCurrentScene()
{
	if (IsValid(_CurrentSceneInstance))
	{
		_CurrentSceneInstance->ExitScene();
	}

	_CurrentSceneInstance = nullptr;
}

void UStoryFlowSubsystem::ClearCurrentBranch()
{
	_CurrentBranchNode = nullptr;
}

void UStoryFlowSubsystem::ClearCurrentShot()
{
	if (IsValid(_CurrentShotInstance))
	{
		_CurrentShotInstance->ExitShot();
	}

	_CurrentShotInstance = nullptr;
	_CurrentShotNode = nullptr;
}

FStoryFlowRef UStoryFlowSubsystem::GetCurrentRef() const
{
	FStoryFlowRef current_ref;
	if (IsValid(_CurrentSceneAsset))
	{
		current_ref.SceneID = _CurrentSceneAsset->GetSceneID();
	}

	if (IsValid(_CurrentShotNode))
	{
		current_ref.ShotID = _CurrentShotNode->GetShotID();
	}

	return current_ref;
}

float UStoryFlowSubsystem::GetTargetLevelLoadingProgressRate() const
{
	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::None)
	{
		return 0.0f;
	}

	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::OpeningTargetLevel)
	{
		return 1.0f;
	}

	float target_level_load_progress = 0.0f;
	if (_PendingTargetLevelLoadCompleted)
	{
		target_level_load_progress = 1.0f;
	}
	if (_PendingTargetLevelLoadHandle.IsValid())
	{
		target_level_load_progress = _PendingTargetLevelLoadHandle->GetLoadProgress();
	}

	return FMath::Min(target_level_load_progress, GetMinimumLoadingTimeProgress());
}
