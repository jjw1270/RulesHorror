// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_SitePanel.h"
#include "SaveGame/SaveGameSubsystem.h"
#include "Item/RulesHorrorItemHelper.h"
#include "UI/Common/RadioButtonGroup_Index.h"
#include "Components/UniformGridPanel.h"
#include "BTN_StoryTitle.h"

TOptional<int32> UUI_SitePanel::_LastRadioButtonIndex;

void UUI_SitePanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(RadioButtonGroup_Index))
	{
		RadioButtonGroup_Index->_OnRadioButtonSelected.AddDynamic(this, &UUI_SitePanel::OnRadioButtonSelected);
	}
}

void UUI_SitePanel::NativeConstruct()
{
	Super::NativeConstruct();

	const auto save_game = USaveGameHelper::GetSaveGame(this);
	if (IsValid(save_game))
	{
		SetNickName(FText::FromString(save_game->GetNickName()));
	}

	_AllStroyItemRows = URulesHorrorItemHelper::GetAllStoryItemRows();

	_StoryTitleNum = 0;
	if (IsValid(UGP_Story))
	{
		for (auto child : UGP_Story->GetAllChildren())
		{
			auto story_title = Cast<UBTN_StoryTitle>(child);
			if (IsInvalid(story_title))
			{
				TRACE_ERROR(TEXT("UGP_Story 에 StoryTitle만 들어있을 수 있습니다."));
				continue;
			}

			_StoryTitleNum++;
		}
	}

	if (_LastRadioButtonIndex.IsSet() == false)
	{
		_LastRadioButtonIndex = 0;
	}

	if (IsValid(RadioButtonGroup_Index))
	{
		int32 max_radio_button_idx = _AllStroyItemRows.Num() / _StoryTitleNum + 1;

		RadioButtonGroup_Index->InitWidget(max_radio_button_idx, _LastRadioButtonIndex.GetValue());
	}
}

void UUI_SitePanel::UpdateStoryTitles()
{
	int32 start_idx = _StoryTitleNum * _LastRadioButtonIndex.GetValue();

	for (int32 i = 0; i < _StoryTitleNum; ++i)
	{
		auto story_title = Cast<UBTN_StoryTitle>(UGP_Story->GetChildAt(i));
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

void UUI_SitePanel::OnRadioButtonSelected(URadioButton* _btn)
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
