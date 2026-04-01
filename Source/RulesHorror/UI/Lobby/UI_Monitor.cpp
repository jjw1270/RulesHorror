// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_Monitor.h"
#include "Components/WidgetSwitcher.h"
#include "UI/Lobby/UI_Cursor.h"
#include "Components/CanvasPanelSlot.h"

TOptional<int32> UUI_Monitor::_LastActiveWidgetIndex;

void UUI_Monitor::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(WidgetSwitcher))
	{
		for (auto child : WidgetSwitcher->GetAllChildren())
		{
			auto widget = Cast<UWidgetBase>(child);
			if (IsInvalid(widget))
				continue;

			widget->Hide(EWidgetHideType::Collapsed, true);
		}
	}
}

void UUI_Monitor::NativeConstruct()
{
	Super::NativeConstruct();

	ShowMonitorCursor(false);
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

void UUI_Monitor::ShowMonitorCursor(bool _is_show)
{
	if (_is_show)
	{
		UI_Cursor->Show(EWidgetShowType::HitTestInvisible);
	}
	else
	{
		UI_Cursor->Hide(EWidgetHideType::Collapsed);
	}
}

void UUI_Monitor::SetMonitorCursorPosition(const FVector2D& _pos)
{
	auto cp_slot = Cast<UCanvasPanelSlot>(UI_Cursor->Slot);
	if (IsInvalid(cp_slot))
		return;

	cp_slot->SetPosition(_pos);
}
