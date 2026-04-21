// Copyright (c) 2026 장윤제. All rights reserved.


#include "Site_StoryDetail.h"
#include "Item/RulesHorrorItemHelper.h"
#include "Widgets/Components/ClickButton.h"
#include "Kismet/GameplayStatics.h"

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

	const auto& story_item_row = URulesHorrorItemHelper::GetStoryItemRow(_CurrentStoryID);
	
	if (story_item_row.StoryMap.IsNull())
		return;

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, story_item_row.StoryMap);
}
