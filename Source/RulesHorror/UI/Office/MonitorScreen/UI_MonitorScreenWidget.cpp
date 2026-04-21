// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_MonitorScreenWidget.h"

void UUI_MonitorScreenWidget::RequestShowMonitorCursor(bool _show, bool _set_cursor_center)
{
	_RequestShowMonitorCursorEvent.ExecuteIfBound(_show, _set_cursor_center);
}
