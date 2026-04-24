// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlowSubsystem.h"
#include "StorySceneAsset.h"
#include "StorySceneBase.h"
#include "StorySceneNodeData.h"
#include "StoryFlowDeveloperSettings.h"
#include "StorySceneRegistryAsset.h"
#include "StoryShotBase.h"
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

void UStoryFlowSubsystem::Tick(float _delta_time)
{
	if (IsInvalid(_CurrentShotInstance))
	{
		return;
	}

	_CurrentShotInstance->TickShot(_delta_time);
	if (_CurrentShotInstance->IsFinished())
	{
		if (MoveToNextShot() == false)
		{
			StopScene();
		}
	}
}

TStatId UStoryFlowSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStoryFlowSubsystem, STATGROUP_Tickables);
}

bool UStoryFlowSubsystem::IsTickable() const
{
	return IsValid(_CurrentShotInstance);
}

bool UStoryFlowSubsystem::StartFromScene(const FStorySceneID& _scene_id)
{
	if (_scene_id.IsValid() == false)
	{
		return false;
	}

	FStoryFlowRef story_flow_ref;
	story_flow_ref.SceneID = _scene_id;
	return StartFromRef(story_flow_ref);
}

bool UStoryFlowSubsystem::StartFromRef(const FStoryFlowRef& _story_flow_ref)
{
	UStorySceneAsset* scene_asset = FindSceneAssetBySceneID(_story_flow_ref.SceneID);

	if (IsInvalid(scene_asset))
	{
		return false;
	}

	if (ShouldOpenTargetLevel(scene_asset))
	{
		StopScene();

		_PendingStartRef = _story_flow_ref;
		_PendingStartRef.SceneID = scene_asset->GetSceneID();
		_PendingTargetLevel = scene_asset->GetTargetLevel();

		const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();
		const bool should_use_loading_level = IsValid(settings)
			&& settings->_LoadingLevel.IsNull() == false
			&& GetLevelPackageName(settings->_LoadingLevel) != GetLevelPackageName(_PendingTargetLevel);

		_PendingTravelPhase = should_use_loading_level
			? EStoryFlowPendingTravelPhase::LoadingLevel
			: EStoryFlowPendingTravelPhase::TargetLevel;

		RequestOpenPendingLevel();
		return true;
	}

	return StartResolvedScene(scene_asset, _story_flow_ref);
}

void UStoryFlowSubsystem::StopScene()
{
	ClearCurrentShot();
	ClearCurrentScene();
	_CurrentSceneAsset = nullptr;
}

UStorySceneAsset* UStoryFlowSubsystem::FindSceneAssetBySceneID(const FStorySceneID& _scene_id) const
{
	if (_scene_id.IsValid() == false)
	{
		return nullptr;
	}

	const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();
	if (IsInvalid(settings) || settings->_StorySceneRegistry.IsNull())
	{
		return nullptr;
	}

	UStorySceneRegistryAsset* scene_registry = settings->_StorySceneRegistry.LoadSynchronous();
	if (IsInvalid(scene_registry))
	{
		return nullptr;
	}

	return scene_registry->FindSceneAsset(_scene_id);
}

bool UStoryFlowSubsystem::StartResolvedScene(UStorySceneAsset* _scene_asset, const FStoryFlowRef& _story_flow_ref)
{
	if (IsInvalid(_scene_asset))
	{
		return false;
	}

	StopScene();
	_CurrentSceneAsset = _scene_asset;

	FStoryFlowRef start_ref = _story_flow_ref;
	start_ref.SceneID = _CurrentSceneAsset->GetSceneID();
	start_ref.ShotID = start_ref.ShotID.IsValid() ? start_ref.ShotID : _CurrentSceneAsset->GetEntryShotID();

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
	{
		return false;
	}

	return GetCurrentLevelPackageName() != GetLevelPackageName(_scene_asset->GetTargetLevel());
}

void UStoryFlowSubsystem::RequestOpenPendingLevel()
{
	const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();

	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::LoadingLevel
		&& IsValid(settings)
		&& settings->_LoadingLevel.IsNull() == false)
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, settings->_LoadingLevel);
		return;
	}

	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::TargetLevel
		&& _PendingTargetLevel.IsNull() == false)
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, _PendingTargetLevel);
		return;
	}

	ClearPendingSceneStart();
}

void UStoryFlowSubsystem::ClearPendingSceneStart()
{
	_PendingStartRef.Reset();
	_PendingTargetLevel = nullptr;
	_PendingTravelPhase = EStoryFlowPendingTravelPhase::None;
}

void UStoryFlowSubsystem::HandlePostLoadMap(UWorld* _loaded_world)
{
	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::None || IsInvalid(_loaded_world))
	{
		return;
	}

	const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();
	const FString loaded_world_package_name = GetWorldPackageName(_loaded_world);

	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::LoadingLevel)
	{
		const FString loading_level_package_name = IsValid(settings) ? GetLevelPackageName(settings->_LoadingLevel) : FString();
		if (loading_level_package_name.IsEmpty() || loaded_world_package_name != loading_level_package_name)
		{
			return;
		}

		_PendingTravelPhase = EStoryFlowPendingTravelPhase::TargetLevel;
		RequestOpenPendingLevel();
		return;
	}

	if (_PendingTravelPhase == EStoryFlowPendingTravelPhase::TargetLevel)
	{
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

	return UWorld::RemovePIEPrefix(_world->GetOutermost()->GetName());
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

bool UStoryFlowSubsystem::MoveToNextShot()
{
	if (IsInvalid(_CurrentShotNode))
	{
		return false;
	}

	const TArray<FStoryShotID>& next_shot_ids = _CurrentShotNode->GetNextShotIDs();
	if (next_shot_ids.Num() == 0)
	{
		return false;
	}

	return MoveToShot(next_shot_ids[0]);
}

void UStoryFlowSubsystem::ClearCurrentScene()
{
	if (IsValid(_CurrentSceneInstance))
	{
		_CurrentSceneInstance->ExitScene();
	}

	_CurrentSceneInstance = nullptr;
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
