// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBase.h"
#include "UI_Monitor.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_Monitor : public UWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnShow_Implementation() override;

#pragma region MonitorWidget
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidgetSwitcher> WidgetSwitcher = nullptr;
	
protected:
	static TOptional<int32> _LastActiveWidgetIndex;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere)
	int32 _DEBUG_Start_Widget_Index = -1;
#endif

#if WITH_EDITOR
public:
	static void ResetLastActiveWidgetindex() { _LastActiveWidgetIndex.Reset(); }
#endif

protected:
	UFUNCTION()
	void OnShowScreenWidget(UWidgetBase* _widget);

	UFUNCTION()
	void OnCloseScreenWidget(UWidgetBase* _widget);

	void ShowNextScreenWidget();

public:
	class UUI_MonitorScreenWidget* GetCurrentScreenWidget() const;

#pragma endregion MonitorWidget
////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Cursor
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UUI_Cursor> UI_Cursor = nullptr;

	bool _RealMousePointerHovered = false;

public:
	void SetRealMousePointerHovered(bool _value)
	{
		_RealMousePointerHovered = _value;
	}

	void ShowMonitorCursor(bool _is_show, bool _set_cursor_center);
	void SetMonitorCursorPosition(const FVector2D& _pos = FVector2D::ZeroVector);

#pragma endregion Cursor
};
