// Copyright (c) 2026 장윤제. All rights reserved.


#include "Objects/Story/HubBrandStageComponent.h"
#include "SaveGame/RulesHorrorProgressSubsystem.h"
#include "RulesHorrorUtils.h"

UHubBrandStageComponent::UHubBrandStageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHubBrandStageComponent::BeginPlay()
{
	Super::BeginPlay();

	if (URulesHorrorProgressSubsystem* progress = URulesHorrorUtils::GetGameInstanceSubsystem<URulesHorrorProgressSubsystem>(this))
	{
		progress->_OnProgressFlagChanged.AddUniqueDynamic(this, &UHubBrandStageComponent::OnProgressFlagChanged);
	}

	RefreshStage();
}

void UHubBrandStageComponent::EndPlay(const EEndPlayReason::Type _end_play_reason)
{
	if (URulesHorrorProgressSubsystem* progress = URulesHorrorUtils::GetGameInstanceSubsystem<URulesHorrorProgressSubsystem>(this))
	{
		progress->_OnProgressFlagChanged.RemoveDynamic(this, &UHubBrandStageComponent::OnProgressFlagChanged);
	}

	Super::EndPlay(_end_play_reason);
}

void UHubBrandStageComponent::RefreshStage()
{
	AActor* owner = GetOwner();
	if (IsInvalid(owner))
		return;

	const URulesHorrorProgressSubsystem* progress = URulesHorrorUtils::GetGameInstanceSubsystem<URulesHorrorProgressSubsystem>(this);
	const int32 brand_count = IsValid(progress) ? progress->GetBrandCount() : 0;
	const bool is_active = IsActiveForBrandCount(brand_count);

	owner->SetActorHiddenInGame(is_active == false);
	if (_DisableCollisionWhenHidden)
	{
		owner->SetActorEnableCollision(is_active);
	}
}

bool UHubBrandStageComponent::IsActiveForBrandCount(int32 _brand_count) const
{
	return _brand_count >= _MinBrandCount && (_MaxBrandCount < 0 || _brand_count <= _MaxBrandCount);
}

void UHubBrandStageComponent::OnProgressFlagChanged(ERulesHorrorProgressFlag _flag, bool _value)
{
	switch (_flag)
	{
	case ERulesHorrorProgressFlag::Brand_01_Oppression:
	case ERulesHorrorProgressFlag::Brand_02_Bystanding:
	case ERulesHorrorProgressFlag::Brand_03_Exploitation:
		RefreshStage();
		break;
	default:
		break;
	}
}
