// Copyright (c) 2026 장윤제. All rights reserved.


#include "BTN_WindowIcon.h"

void UBTN_WindowIcon::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	SetIconImage(_IconImage.LoadSynchronous());
	SetIconText(_IconText);
}
