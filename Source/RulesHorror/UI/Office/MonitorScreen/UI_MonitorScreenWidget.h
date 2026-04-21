// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBase.h"
#include "UI_MonitorScreenWidget.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_MonitorScreenWidget : public UWidgetBase
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_TwoParams(F_RequestShowMonitorCursor, bool, bool);
	
	F_RequestShowMonitorCursor _RequestShowMonitorCursorEvent;

protected:
	UPROPERTY(EditAnywhere)
	bool _ShowMouseCursorOnShow = false;

protected:
	UFUNCTION(BlueprintCallable)
	void RequestShowMonitorCursor(bool _show, bool _set_cursor_center = false);

public:
	bool GetShowMouseCursorOnShow() const { return _ShowMouseCursorOnShow; }
};
