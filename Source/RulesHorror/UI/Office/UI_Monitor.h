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

public:
	/* 다음에 모니터를 켤 때 첫 화면(로딩 → 닉네임)부터 보여 준다. 새 게임/이어하기, PIE 종료 때 부른다. */
	static void ResetLastActiveWidgetindex() { _LastActiveWidgetIndex.Reset(); }

protected:
	UFUNCTION()
	void OnShowScreenWidget(UWidgetBase* _widget);

	UFUNCTION()
	void OnHideScreenWidget(UWidgetBase* _widget, EWidgetHideType _hide_type);

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
