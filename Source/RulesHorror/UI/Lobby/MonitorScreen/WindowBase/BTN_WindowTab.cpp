// Copyright (c) 2026 장윤제. All rights reserved.


#include "BTN_WindowTab.h"


void UBTN_WindowTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	_NonHighlightedStateStyles = _StateStyles;
}

void UBTN_WindowTab::InitWidget(EWindowWidgetType _type, UTexture2D* _tab_image, const FText& _tab_text)
{
	_WindowWidgetType = _type;

	SetTabIcon(_tab_image);
	SetTabText(_tab_text);
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
