// Fill out your copyright notice in the Description page of Project Settings.


#include "Window_Explorer.h"
#include "UI/Lobby/Explorer/UI_SitePanel.h"

void UWindow_Explorer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(UI_SitePanel))
	{
		UI_SitePanel->_OnSiteChangedEvent.AddDynamic(this, &UWindow_Explorer::SetAddressText);
	}
}
