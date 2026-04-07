// Copyright (c) 2026 장윤제. All rights reserved.


#include "BTN_WindowTab.h"


void UBTN_WindowTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	_NonHighlightedStateStyles = _StateStyles;
}

void UBTN_WindowTab::SetHighlight(bool _is_highlighted)
{
	if(_is_highlighted)
	{
		_StateStyles = _HighlightedStateStyles;
	}
	else
	{
		_StateStyles = _NonHighlightedStateStyles;
	}

	SynchronizeProperties();
}
