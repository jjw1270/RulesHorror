// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_WindowLayout.h"


void UUI_WindowLayout::RequestCommand(EWindowCommand _command)
{
	_OnRequestCommandEvent.ExecuteIfBound(_command);
}
