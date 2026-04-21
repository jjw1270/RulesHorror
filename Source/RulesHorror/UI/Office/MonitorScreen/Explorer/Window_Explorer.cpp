// Copyright (c) 2026 장윤제. All rights reserved.


#include "Window_Explorer.h"
#include "UI/Office/MonitorScreen/Explorer/UI_SitePanel.h"

void UWindow_Explorer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(UI_SitePanel))
	{
		UI_SitePanel->_OnSiteChangedEvent.AddDynamic(this, &UWindow_Explorer::SetAddressText);
	}
}
