// Copyright (c) 2026 장윤제. All rights reserved.

#include "UI_WindowManager.h"
#include "UI/Lobby/MonitorScreen/WindowBase/WindowBase.h"
#include "UI/Lobby/MonitorScreen/WindowBase/BTN_WindowTab.h"
#include "UI/Lobby/MonitorScreen/WindowBase/BTN_WindowIcon.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/UniformGridPanel.h"


void UUI_WindowManager::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(UGP_Icons))
	{
		for (auto child : UGP_Icons->GetAllChildren())
		{
			auto window_icon = Cast<UBTN_WindowIcon>(child);
			if (IsInvalid(window_icon))
				continue;

			window_icon->_OnDoubleClicked.AddDynamic(this, &UUI_WindowManager::OnDoubleClickIconButton);
		}
	}
}

void UUI_WindowManager::OpenWindow(EWindowWidgetType _type)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr))
		return;

	if (IsInvalid(data_ptr->WindowWidget))
	{
		if (CreateWindowWidget(_type, *data_ptr) == false)
		{
			TRACE_ERROR(TEXT("Create Window 실패"));
			return;
		}
	}

	auto window_widget = data_ptr->WindowWidget;

	if (IsInvalid(window_widget->GetParent()))
	{
		auto cp_slot = CP_Window->AddChildToCanvas(window_widget);
		if (IsValid(cp_slot))
		{
			cp_slot->SetAutoSize(false);
			cp_slot->SetAlignment(FVector2D::ZeroVector);
			cp_slot->SetSize(_DefaultWindowSize);
		}
	}

	window_widget->Show(EWidgetShowType::SelfHitTestInvisible);

	if (!data_ptr->HasBeenOpened && data_ptr->IsMaximized)
	{
		window_widget->SetMaximize(true);
	}

	data_ptr->HasBeenOpened = true;
	SetTopWindow(window_widget);

	UpdateTopWindow();
}

bool UUI_WindowManager::CreateWindowWidget(EWindowWidgetType _type, FWindowData& _window_data)
{
	if (IsInvalid(_window_data.WindowWidgetClass))
	{
		TRACE_ERROR(TEXT("WindowWidgetClass setting 오류."));
		return false;
	}

	_window_data.WindowWidget = CreateWidget<UWindowBase>(this, _window_data.WindowWidgetClass);
	if (IsInvalid(_window_data.WindowWidget))
	{
		TRACE_ERROR(TEXT("window widget 생성 실패."));
		return false;
	}

	_window_data.WindowWidget->InitWidget(_type, GetIdealWindowPosition());
	_window_data.WindowWidget->_OnWindowFocusedEvent.AddDynamic(this, &UUI_WindowManager::OnFocusedWindowWidget);
	_window_data.WindowWidget->_OnStartHideEvent.AddDynamic(this, &UUI_WindowManager::OnStartHideWindowWidget);

	return true;
}

bool UUI_WindowManager::CreateWindowTab(EWindowWidgetType _type, FWindowData& _window_data)
{
	if (IsInvalid(_WindowTabClass))
	{
		TRACE_ERROR(TEXT("WindowTabClass setting 오류."));
		return false;
	}

	_window_data.WindowTab = CreateWidget<UBTN_WindowTab>(this, _WindowTabClass);
	if (IsInvalid(_window_data.WindowTab))
	{
		TRACE_ERROR(TEXT("window tab 생성 실패."));
		return false;
	}

	_window_data.WindowTab->InitWidget(_type, _window_data.TabImage, _window_data.TabText);
	_window_data.WindowTab->_OnClicked.AddDynamic(this, &UUI_WindowManager::OnClickWindowTab);

	return true;
}

void UUI_WindowManager::CloseWindow(EWindowWidgetType _type, bool _is_minimized)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsAnyInvalid(data_ptr, data_ptr->WindowWidget))
		return;

	if (_is_minimized)
	{
		data_ptr->WindowWidget->Hide(EWidgetHideType::Collapsed);
	}
	else
	{
		data_ptr->WindowWidget->Close();
	}
}

void UUI_WindowManager::SetTopWindow(UWindowBase* _target_window)
{
	if (IsAnyInvalid(CP_Window, _target_window))
		return;

	auto target_slot = Cast<UCanvasPanelSlot>(_target_window->Slot);
	if (IsInvalid(target_slot))
		return;

	const int32 max_z_order = GetMaxWindowZOrder();

	if (target_slot->GetZOrder() < max_z_order)
	{
		// 계속 쌓이겠지만, 오버플로나려면 한참해야할거다..
		target_slot->SetZOrder(max_z_order + 1);
	}

	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		const bool is_top = (window_widget == _target_window);

		auto data_ptr = _WindowDataMap.Find(window_widget->GetWindowWidgetType());
		if (IsAllValid(data_ptr, data_ptr->WindowTab))
		{
			data_ptr->WindowTab->SetHighlight(is_top);
		}
	}
}

void UUI_WindowManager::UpdateTopWindow()
{
	UWindowBase* top_window = GetTopWindow();
	if (IsValid(top_window) && top_window->IsVisible())
		return;

	if (IsInvalid(CP_Window))
		return;

	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget) || !window_widget->IsVisible())
			continue;

		SetTopWindow(window_widget);
		return;
	}
}

UWindowBase* UUI_WindowManager::GetTopWindow() const
{
	if (IsInvalid(CP_Window))
		return nullptr;

	UWindowBase* top_window = nullptr;
	int32 top_z_order = TNumericLimits<int32>::Lowest();

	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsInvalid(cp_slot))
			continue;

		const int32 z_order = cp_slot->GetZOrder();
		if (z_order > top_z_order)
		{
			top_z_order = z_order;
			top_window = window_widget;
		}
	}

	return top_window;
}

void UUI_WindowManager::OnDoubleClickIconButton(UClickButton* _icon_button)
{
	auto window_icon = Cast<UBTN_WindowIcon>(_icon_button);
	if (IsInvalid(window_icon))
		return;

	const EWindowWidgetType window_widget_type = window_icon->GetWindowWidgetType();

	auto window_data_ptr = _WindowDataMap.Find(window_widget_type);
	if (IsInvalid(window_data_ptr))
		return;
	
	if (IsInvalid(window_data_ptr->WindowTab))
	{
		CreateWindowTab(window_widget_type, *window_data_ptr);
	}

	if (IsValid(window_data_ptr->WindowTab))
	{
		HB_WindowTab->AddChildToHorizontalBox(window_data_ptr->WindowTab);
	}

	OpenWindow(window_widget_type);
}

void UUI_WindowManager::OnClickWindowTab(UButtonBase* _tab_button)
{
	auto window_tab = Cast<UBTN_WindowTab>(_tab_button);
	if (IsInvalid(window_tab))
		return;

	const EWindowWidgetType window_widget_type = window_tab->GetWindowWidgetType();

	auto window_data_ptr = _WindowDataMap.Find(window_widget_type);
	if (IsAnyInvalid(window_data_ptr, window_data_ptr->WindowWidget))
		return;

	auto window_widget = window_data_ptr->WindowWidget;

	const EWidgetState widget_state = window_widget->GetWidgetState();

	if (IsInvalid(window_widget->GetParent()) || widget_state == EWidgetState::Hiding || widget_state == EWidgetState::Hide)
	{
		OpenWindow(window_widget_type);
	}
	else if (window_widget != GetTopWindow())
	{
		SetTopWindow(window_widget);
	}
	else
	{
		CloseWindow(window_widget_type, true);
	}
}

void UUI_WindowManager::OnFocusedWindowWidget(UWindowBase* _focused_window_widget, bool _is_focused)
{
	if (IsInvalid(_focused_window_widget))
		return;

	if (_is_focused)
	{
		SetTopWindow(_focused_window_widget);
	}
	else
	{
		UpdateTopWindow();
	}
}

void UUI_WindowManager::OnStartHideWindowWidget(UWidgetBase* _widget, EWidgetHideType _hide_type)
{
	auto window_widget = Cast<UWindowBase>(_widget);
	if (IsInvalid(window_widget))
		return;

	if (_hide_type == EWidgetHideType::RemoveFromParent)
	{
		auto window_data_ptr = _WindowDataMap.Find(window_widget->GetWindowWidgetType());
		if (IsAnyInvalid(window_data_ptr, window_data_ptr->WindowTab))
			return;

		window_data_ptr->WindowTab->Close();

		window_data_ptr->HasBeenOpened = false;
	}
}

FVector2D UUI_WindowManager::GetIdealWindowPosition() const
{
	UWindowBase* top_window = GetTopWindow();
	if (IsInvalid(top_window) || top_window->IsMaximized())
		return _InitialWindowPos;

	auto top_window_slot = Cast<UCanvasPanelSlot>(top_window->Slot);
	if (IsInvalid(top_window_slot))
		return _InitialWindowPos;

	return top_window_slot->GetPosition() + _WindowOffset;
}

int32 UUI_WindowManager::GetMaxWindowZOrder() const
{
	if (IsInvalid(CP_Window))
		return 0;

	int32 max_z_order = 0;

	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsInvalid(cp_slot))
			continue;

		max_z_order = FMath::Max(max_z_order, cp_slot->GetZOrder());
	}

	return max_z_order;
}
