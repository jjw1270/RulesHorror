// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_Monitor.h"
#include "Components/WidgetSwitcher.h"

int32 UUI_Monitor::_LastActiveWidgetIndex = -1;

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

void UUI_Monitor::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	if (_LastActiveWidgetIndex < 0)
	{
		_LastActiveWidgetIndex = 0;
	}

	WidgetSwitcher->SetActiveWidgetIndex(_LastActiveWidgetIndex);

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
