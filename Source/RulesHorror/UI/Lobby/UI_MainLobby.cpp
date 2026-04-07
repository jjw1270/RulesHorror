// Copyright (c) 2026 장윤제. All rights reserved.

#include "UI_MainLobby.h"
#include "UI/Lobby/WindowBase/WindowBase.h"
#include "UI/Lobby/WindowBase/BTN_WindowTab.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"


void UUI_MainLobby::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsInvalid(CP_Window))
		return;

	FVector2D normal_pos = _InitialNormalPos;

	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		window_widget->SetLastNormalPos(normal_pos);
		normal_pos += _WindowOffset;
	}
}

void UUI_MainLobby::CreateWindow(EWindowWidgetType _type)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr))
		return;

	if (IsValid(data_ptr->WindowWidget))
		return;

	if (IsInvalid(data_ptr->WindowWidgetClass))
	{
		TRACE_ERROR(TEXT("WindowWidgetClass setting 오류."));
		return;
	}

	auto window_widget = CreateWidget<UWindowBase>(this, data_ptr->WindowWidgetClass);
	if (IsInvalid(window_widget))
	{
		TRACE_ERROR(TEXT("window widget 생성 실패."));
		return;
	}

	data_ptr->WindowWidget = window_widget;

	InitializeWindowWidget(window_widget, _type, GetNextWindowPosition());
	AttachWindowToCanvas(window_widget);

	if (data_ptr->CreateTab)
	{
		CreateWindowTab(_type, *data_ptr);
	}
}

void UUI_MainLobby::OpenWindow(EWindowWidgetType _type, bool _is_open)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr) || IsInvalid(data_ptr->WindowWidget))
		return;

	UWindowBase* window_widget = data_ptr->WindowWidget;

	if (_is_open)
	{
		window_widget->Show(EWidgetShowType::SelfHitTestInvisible);

		if (!data_ptr->HasBeenOpened && data_ptr->IsMaximized)
		{
			window_widget->SetMaximize(true);
		}

		data_ptr->HasBeenOpened = true;
		SetTopWindow(window_widget);
		return;
	}

	window_widget->Hide(EWidgetHideType::Collapsed);
	UpdateTopWindow();
}

void UUI_MainLobby::SetTopWindow(UWindowBase* _target_window)
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

		auto data_ptr = _WindowDataMap.Find(window_widget->_WindowWidgetType);
		if (IsAllValid(data_ptr, data_ptr->WindowTab))
		{
			data_ptr->WindowTab->SetHighlight(is_top);
		}
	}
}

void UUI_MainLobby::UpdateTopWindow()
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

UWindowBase* UUI_MainLobby::GetTopWindow() const
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

void UUI_MainLobby::OnClickWindowTab(UButtonBase* _tab_button)
{
	auto window_tab = Cast<UBTN_WindowTab>(_tab_button);
	if (IsInvalid(window_tab))
		return;

	const EWindowWidgetType window_widget_type = window_tab->_WindowWidgetType;

	auto window_data_ptr = _WindowDataMap.Find(window_widget_type);
	if (IsAnyInvalid(window_data_ptr, window_data_ptr->WindowWidget))
		return;

	auto window_widget = window_data_ptr->WindowWidget;

	if (IsInvalid(window_widget->GetParent()))
	{
		InitializeWindowWidget(window_widget, window_widget_type, GetNextWindowPosition());
		AttachWindowToCanvas(window_widget);

		window_data_ptr->HasBeenOpened = false;
		OpenWindow(window_widget_type, true);
		return;
	}

	switch (window_widget->GetWidgetState())
	{
	case EWidgetState::Hiding:
	case EWidgetState::Hide:
		OpenWindow(window_widget_type, true);
		break;

	case EWidgetState::Showing:
	case EWidgetState::Idle:
		if (window_widget == GetTopWindow())
		{
			OpenWindow(window_widget_type, false);
		}
		else
		{
			SetTopWindow(window_widget);
		}
		break;

	default:
		break;
	}
}

void UUI_MainLobby::OnWindowFocused(UWindowBase* _focused_window_widget, bool _is_focused)
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

FVector2D UUI_MainLobby::GetNextWindowPosition() const
{
	UWindowBase* top_window = GetTopWindow();
	if (IsInvalid(top_window) || top_window->IsMaximized())
		return _InitialWindowPos;

	auto top_window_slot = Cast<UCanvasPanelSlot>(top_window->Slot);
	if (IsInvalid(top_window_slot))
		return _InitialWindowPos;

	return top_window_slot->GetPosition() + _WindowOffset;
}

void UUI_MainLobby::InitializeWindowWidget(UWindowBase* _window_widget, EWindowWidgetType _type, const FVector2D& _window_pos)
{
	if (IsInvalid(_window_widget))
		return;

	_window_widget->_WindowWidgetType = _type;
	_window_widget->SetMaximize(false);
	_window_widget->SetLastNormalPos(_window_pos);
	_window_widget->Hide(EWidgetHideType::Collapsed);

	_window_widget->_OnWindowFocusedEvent.RemoveDynamic(this, &UUI_MainLobby::OnWindowFocused);
	_window_widget->_OnWindowFocusedEvent.AddDynamic(this, &UUI_MainLobby::OnWindowFocused);
}

void UUI_MainLobby::AttachWindowToCanvas(UWindowBase* _window_widget)
{
	if (IsAnyInvalid(CP_Window, _window_widget))
		return;

	auto cp_slot = CP_Window->AddChildToCanvas(_window_widget);
	if (IsInvalid(cp_slot))
		return;

	cp_slot->SetAutoSize(false);
	cp_slot->SetAlignment(FVector2D::ZeroVector);
	cp_slot->SetSize(_DefaultWindowSize);
}

void UUI_MainLobby::CreateWindowTab(EWindowWidgetType _type, FWindowData& _window_data)
{
	if (IsAnyInvalid(HB_WindowTab, _WindowTabClass))
	{
		TRACE_ERROR(TEXT("WindowTabClass setting 오류."));
		return;
	}

	if (IsValid(_window_data.WindowTab))
		return;

	auto window_tab = CreateWidget<UBTN_WindowTab>(this, _WindowTabClass);
	if (IsInvalid(window_tab))
	{
		TRACE_ERROR(TEXT("window tab 생성 실패."));
		return;
	}

	_window_data.WindowTab = window_tab;

	window_tab->_WindowWidgetType = _type;
	window_tab->SetTabIcon(_window_data.WindowTabIcon);
	window_tab->SetTabText(_window_data.WindowTabText);
	window_tab->_OnClicked.AddDynamic(this, &UUI_MainLobby::OnClickWindowTab);

	HB_WindowTab->AddChildToHorizontalBox(window_tab);
}

int32 UUI_MainLobby::GetMaxWindowZOrder() const
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
