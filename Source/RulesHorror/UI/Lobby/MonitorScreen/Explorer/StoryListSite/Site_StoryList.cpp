// Copyright (c) 2026 장윤제. All rights reserved.


#include "Site_StoryList.h"
#include "SaveGameSubsystem.h"
#include "Item/RulesHorrorItemHelper.h"
#include "UI/Common/RadioButtonGroup_Index.h"
#include "Components/UniformGridPanel.h"
#include "BTN_StoryTitle.h"

TOptional<int32> USite_StoryList::_LastRadioButtonIndex;

void USite_StoryList::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(RadioButtonGroup_Index))
	{
		RadioButtonGroup_Index->_OnRadioButtonSelected.AddDynamic(this, &USite_StoryList::OnRadioButtonSelected);
	}

	_StoryTitleNum = 0;

	if (IsValid(UniformGridPanel))
	{
		for (auto child : UniformGridPanel->GetAllChildren())
		{
			auto btn_story_title = Cast<UBTN_StoryTitle>(child);
			if (IsInvalid(btn_story_title))
			{
				TRACE_ERROR(TEXT("UniformGridPanel 에 StoryTitle만 들어있을 수 있습니다."));
				continue;
			}

			btn_story_title->_OnClicked.AddDynamic(this, &USite_StoryList::OnClickStoryTitleButton);

			_StoryTitleNum++;
		}
	}
}

void USite_StoryList::NativeConstruct()
{
	Super::NativeConstruct();

	const auto save_game = USaveGameHelper::GetSaveGame(this);
	if (IsValid(save_game))
	{
		// TODO : Story 진행상황 불러오기
	}

	if (_LastRadioButtonIndex.IsSet() == false)
	{
		_LastRadioButtonIndex = 0;
	}

	_AllStroyItemRows = URulesHorrorItemHelper::GetAllStoryItemRows();

	if (IsValid(RadioButtonGroup_Index))
	{
		int32 max_radio_button_idx = _AllStroyItemRows.Num() / _StoryTitleNum + 1;

		RadioButtonGroup_Index->InitWidget(max_radio_button_idx, _LastRadioButtonIndex.GetValue());
	}
}

void USite_StoryList::UpdateStoryTitles()
{
	int32 start_idx = _StoryTitleNum * _LastRadioButtonIndex.GetValue();

	for (int32 i = 0; i < _StoryTitleNum; ++i)
	{
		auto story_title = Cast<UBTN_StoryTitle>(UniformGridPanel->GetChildAt(i));
		if (IsInvalid(story_title))
			continue;

		const int32 idx = start_idx + i;

		if (_AllStroyItemRows.IsValidIndex(idx))
		{
			story_title->Show(EWidgetShowType::SelfHitTestInvisible, true);
			story_title->SetStoryID(_AllStroyItemRows[idx].ItemID);
		}
		else
		{
			story_title->Hide(EWidgetHideType::Hidden, true);
		}
	}
}

void USite_StoryList::OnClickStoryTitleButton(UButtonBase* _btn)
{
	auto btn_story_title = Cast<UBTN_StoryTitle>(_btn);
	if (IsInvalid(btn_story_title))
		return;

	_OnClickShowStoryDetailEvent.ExecuteIfBound(btn_story_title->GetStoryID());
}

void USite_StoryList::OnRadioButtonSelected(URadioButton* _btn)
{
	auto radio_button = Cast<URadioButton_Index>(_btn);
	if (IsInvalid(radio_button))
		return;

	int32 idx = radio_button->GetIndex();

	if (_LastRadioButtonIndex.IsSet() == false || _LastRadioButtonIndex.GetValue() != idx)
	{
		_LastRadioButtonIndex = idx;
	}

	UpdateStoryTitles();
}
