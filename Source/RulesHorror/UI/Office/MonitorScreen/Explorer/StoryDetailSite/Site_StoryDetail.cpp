// Copyright (c) 2026 장윤제. All rights reserved.


#include "Site_StoryDetail.h"
#include "Item/RulesHorrorItemHelper.h"
#include "Widgets/Components/ClickButton.h"
#include "Kismet/GameplayStatics.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "StoryFlowSubsystem.h"
#include "RulesHorrorUtils.h"

FItemID_Story USite_StoryDetail::_LastStoryID;

void USite_StoryDetail::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(BTN_GoToStoryList))
	{
		BTN_GoToStoryList->_OnClicked.AddDynamic(this, &USite_StoryDetail::OnClickGoToStoryListButton);
	}
}

void USite_StoryDetail::NativeConstruct()
{
	Super::NativeConstruct();

	if (_LastStoryID.IsValid())
	{
		SetStoryID(_LastStoryID);
	}
}

void USite_StoryDetail::OnClickGoToStoryListButton(UButtonBase* _btn)
{
	_LastStoryID = FItemID_Story::Zero;
	_CurrentStoryID = FItemID_Story::Zero;

	_OnClickShowStoryListEvent.ExecuteIfBound();
}

void USite_StoryDetail::SetStoryID(FItemID_Story _story_id)
{
	if (_CurrentStoryID == _story_id)
		return;

	_CurrentStoryID = _story_id;
	_LastStoryID = _story_id;

	const auto& story_item_row = URulesHorrorItemHelper::GetStoryItemRow(_CurrentStoryID);
	
	SetAdditionalSiteAddress(story_item_row.AdditionalSiteAddress);
	SetTitle(story_item_row.GetDisplayName());
}

void USite_StoryDetail::PlayCurrentStory()
{
	if (_CurrentStoryID.IsValid() == false)
		return;

	// 스토리 흐름이 시나리오 선택을 기다리는 중이면 레벨 이동은 StoryFlow가 맡는다 (CH02_S03).
	if (URulesHorrorStorySubsystem::Send(this, RulesHorrorStoryEvents::SiteStoryPlay))
		return;

	// 스토리가 진행 중이거나 다음 레벨로 넘어가는 중이면 기다리는 Shot이 없어도 직접 맵을 열지 않는다 (Scene이 끊기지 않게).
	// 사이트에서 직접 맵을 여는 임시 흐름은 StoryFlow 밖(맵을 바로 연 PIE)에서만 쓴다.
	const UStoryFlowSubsystem* story_flow_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<UStoryFlowSubsystem>(this);
	if (IsValid(story_flow_subsystem) && (IsValid(story_flow_subsystem->GetCurrentShot()) || story_flow_subsystem->GetPendingStartRef().IsValid()))
	{
		TRACE_LOG(TEXT("Story is running. Ignoring direct story map open."));
		return;
	}

	const auto& story_item_row = URulesHorrorItemHelper::GetStoryItemRow(_CurrentStoryID);
	
	if (story_item_row.StoryMap.IsNull())
		return;

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, story_item_row.StoryMap);
}
