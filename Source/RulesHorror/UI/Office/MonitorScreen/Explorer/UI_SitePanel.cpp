// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_SitePanel.h"
#include "UI/Office/MonitorScreen/Explorer/UI_SiteBase.h"
#include "SaveGameSubsystem.h"
#include "CommonUtils.h"
#include "Components/WidgetSwitcher.h"

void UUI_SitePanel::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	const auto save_game = USaveGameHelper::GetCurrentSaveGame_ReadOnly(this);
	if (IsValid(save_game))
	{
		FString nickname;
		save_game->FindSavedStringData(TEXT("Nickname"), nickname);
		SetNickName(FText::FromString(nickname));
	}

	BroadcastOnSiteChangedEvent(IsValid(WS_Site) ? Cast<UUI_SiteBase>(WS_Site->GetActiveWidget()) : nullptr);
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

void UUI_SitePanel::BroadcastOnSiteChangedEvent(UUI_SiteBase* _site)
{
	if (IsInvalid(_site))
	{
		_OnSiteChangedEvent.Broadcast(_MainSiteAddress);
		return;
	}

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
