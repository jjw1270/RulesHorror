// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_Monitor.h"
#include "RulesHorrorUtils.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/WidgetSwitcher.h"
#include "UI/Lobby/UI_Cursor.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Lobby/UI_MainLobby.h"
#include "UI/Lobby/WindowBase/WindowBase.h"

TOptional<int32> UUI_Monitor::_LastActiveWidgetIndex;

void UUI_Monitor::NativeConstruct()
{
	Super::NativeConstruct();

	ShowMonitorCursor(false, false);
}

void UUI_Monitor::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	if (_LastActiveWidgetIndex.IsSet() == false)
	{
#if WITH_EDITOR
		_LastActiveWidgetIndex = (_DEBUG_Start_Widget_Index > 0) ? _DEBUG_Start_Widget_Index : 0;
#else
		_LastActiveWidgetIndex = 0;
#endif
	}

	WidgetSwitcher->SetActiveWidgetIndex(_LastActiveWidgetIndex.GetValue());

	auto actived_widget = Cast<UWidgetBase>(WidgetSwitcher->GetActiveWidget());
	if (IsValid(actived_widget))
	{
		actived_widget->Hide(EWidgetHideType::Collapsed, true);
		actived_widget->Show(EWidgetShowType::SelfHitTestInvisible);
	}
}

void UUI_Monitor::SetActiveWidgetAndShow(UWidgetBase* _widget, bool _is_skip_anim)
{
	if (IsInvalid(_widget))
		return;

	int32 idx = WidgetSwitcher->GetChildIndex(_widget);
	if (idx == INDEX_NONE)
	{
		TRACE_WARNING(TEXT("WidgetSwitcher에 있는 위젯이어야 합니다."));
		return;
	}

	WidgetSwitcher->SetActiveWidgetIndex(idx);
	_widget->Show(EWidgetShowType::SelfHitTestInvisible, _is_skip_anim);

	_LastActiveWidgetIndex = idx;
}

UWidgetBase* UUI_Monitor::GetCurrentWidget() const
{
	return Cast<UWidgetBase>(WidgetSwitcher->GetActiveWidget());
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

	// update drag on main lobby
	auto main_lobby = Cast<UUI_MainLobby>(GetCurrentWidget());
	if (IsValid(main_lobby))
	{
		auto top_window = main_lobby->GetTopWindow();
		if (IsValid(top_window))
		{
			top_window->UpdateDrag(_pos);
		}
	}
}
