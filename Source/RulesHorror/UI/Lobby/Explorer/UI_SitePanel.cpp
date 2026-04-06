// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_SitePanel.h"
#include "SaveGame/SaveGameSubsystem.h"
#include "Item/RulesHorrorItemHelper.h"
#include "Components/WidgetSwitcher.h"
#include "UI/Lobby/Explorer/StoryListSite/Site_StoryList.h"
#include "UI/Lobby/Explorer/StoryDetailSite/Site_StoryDetail.h"

void UUI_SitePanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(Site_StoryList))
	{
		Site_StoryList->_OnClickShowStoryDetailEvent.BindUObject(this, &UUI_SitePanel::ShowStoryDetailSite);
	}

	if (IsValid(Site_StoryDetail))
	{
		Site_StoryDetail->_OnClickShowStoryListEvent.BindUObject(this, &UUI_SitePanel::ShowStoryListSite);
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

	BroadcastOnSiteChangedEvent(Site_StoryList);
}

void UUI_SitePanel::ChangeSite(UUI_SiteBase* _new_site)
{
	if (IsInvalid(_new_site))
		return;

	const int32 prev_ws_index = WS_Site->GetActiveWidgetIndex();

	WS_Site->SetActiveWidget(_new_site);

	if (prev_ws_index != WS_Site->GetActiveWidgetIndex())
	{
		BroadcastOnSiteChangedEvent(_new_site);
	}
}

void UUI_SitePanel::ShowStoryListSite()
{
	ChangeSite(Site_StoryList);
}

void UUI_SitePanel::BroadcastOnSiteChangedEvent(UUI_SiteBase* _site)
{
	const FString& additional_address = _site->GetAdditionalSiteAddress();
	if (additional_address.IsEmpty() == false)
	{
		_OnSiteChangedEvent.Broadcast(_MainSiteAddress + "/" + additional_address);
	}
	else
	{
		_OnSiteChangedEvent.Broadcast(_MainSiteAddress);
	}
}

void UUI_SitePanel::ShowStoryDetailSite(const FItemID_Story& _story_id)
{
	if (_story_id.IsValid() == false)
		return;

	Site_StoryDetail->SetStoryID(_story_id);
	ChangeSite(Site_StoryDetail);
}
