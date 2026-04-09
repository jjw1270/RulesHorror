// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_Monitor.h"
#include "RulesHorrorUtils.h"
#include "Components/WidgetSwitcher.h"
#include "UI/Lobby/MonitorScreen/UI_MonitorScreenWidget.h"
#include "UI/Lobby/UI_Cursor.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Lobby/MonitorScreen/UI_WindowManager.h"
#include "UI/Lobby/MonitorScreen/WindowBase/WindowBase.h"

TOptional<int32> UUI_Monitor::_LastActiveWidgetIndex;

void UUI_Monitor::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(WidgetSwitcher))
	{
		for (auto child : WidgetSwitcher->GetAllChildren())
		{
			auto screen_widget = Cast<UUI_MonitorScreenWidget>(child);
			if (IsInvalid(screen_widget))
			{
				TRACE_ERROR(TEXT("UUI_MonitorScreenWidget 만 WidgetSwitcher의 child로 들어올 수 있습니다."));
				return;
			}

			screen_widget->Hide(EWidgetHideType::Collapsed, true);

			screen_widget->_RequestShowMonitorCursorEvent.BindUObject(this, &UUI_Monitor::ShowMonitorCursor);
			screen_widget->_OnShowEvent.AddDynamic(this, &UUI_Monitor::OnShowScreenWidget);
			screen_widget->_OnCloseEvent.AddDynamic(this, &UUI_Monitor::OnCloseScreenWidget);
		}
	}
}

void UUI_Monitor::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	ShowMonitorCursor(false, false);

	if (_LastActiveWidgetIndex.IsSet() == false)
	{
#if WITH_EDITOR
		_LastActiveWidgetIndex = (_DEBUG_Start_Widget_Index > 0) ? _DEBUG_Start_Widget_Index : 0;
#else
		_LastActiveWidgetIndex = 0;
#endif
	}

	WidgetSwitcher->SetActiveWidgetIndex(_LastActiveWidgetIndex.GetValue());

	auto actived_screen_widget = Cast<UUI_MonitorScreenWidget>(WidgetSwitcher->GetActiveWidget());
	if (IsValid(actived_screen_widget))
	{
		actived_screen_widget->Show(EWidgetShowType::SelfHitTestInvisible);
	}
}

void UUI_Monitor::OnShowScreenWidget(UWidgetBase* _widget)
{
	auto screen_widget = Cast<UUI_MonitorScreenWidget>(_widget);
	if (IsInvalid(screen_widget))
		return;

	ShowMonitorCursor(screen_widget->GetShowMouseCursorOnShow(), false);
}

void UUI_Monitor::OnCloseScreenWidget(UWidgetBase* _widget, bool _is_removed)
{
	if (IsInvalid(_widget))
		return;

	ShowNextScreenWidget();
}

void UUI_Monitor::ShowNextScreenWidget()
{
	const int32 next_idx = _LastActiveWidgetIndex.GetValue() + 1;

	// 마지막 위젯이면 stop
	if (next_idx >= WidgetSwitcher->GetChildrenCount())
		return;

	WidgetSwitcher->SetActiveWidgetIndex(next_idx);
	_LastActiveWidgetIndex = next_idx;

	auto widget = GetCurrentScreenWidget();
	if (IsInvalid(widget))
		return;

	widget->Show(EWidgetShowType::SelfHitTestInvisible);
}

UUI_MonitorScreenWidget* UUI_Monitor::GetCurrentScreenWidget() const
{
	return Cast<UUI_MonitorScreenWidget>(WidgetSwitcher->GetActiveWidget());
}

void UUI_Monitor::ShowMonitorCursor(bool _is_show, bool _set_cursor_center)
{
	if (_is_show)
	{
		UI_Cursor->Show(EWidgetShowType::HitTestInvisible);

		if (_set_cursor_center)
		{
			const auto& geometry = GetCachedGeometry();
			const FVector2D local_center = geometry.GetLocalSize() * 0.5f;

			if (_RealMousePointerHovered)
			{
				auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
				if (IsValid(pc))
				{
					// 완벽히 일치하진 않고 근사값
					int32 viewport_x, viewport_y;
					pc->GetViewportSize(viewport_x, viewport_y);

					pc->SetMouseLocation(viewport_x / 2, viewport_y / 2);
				}
			}
			else
			{
				auto cursor_slot = Cast<UCanvasPanelSlot>(UI_Cursor->Slot);
				if (IsValid(cursor_slot))
				{
					cursor_slot->SetPosition(local_center);
				}
			}
		}
	}
	else
	{
		UI_Cursor->Hide(EWidgetHideType::Collapsed);
	}
}

void UUI_Monitor::SetMonitorCursorPosition(const FVector2D& _pos)
{
	if (UI_Cursor->IsVisible() == false)
		return;

	auto cp_slot = Cast<UCanvasPanelSlot>(UI_Cursor->Slot);
	if (IsValid(cp_slot))
	{
		cp_slot->SetPosition(_pos);
	}

	// update drag on window manager
	auto window_manager = Cast<UUI_WindowManager>(GetCurrentScreenWidget());
	if (IsValid(window_manager))
	{
		auto top_window = window_manager->GetTopWindow();
		if (IsValid(top_window))
		{
			top_window->UpdateDrag(_pos);
		}
	}
}
