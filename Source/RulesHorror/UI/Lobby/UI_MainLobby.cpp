// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_MainLobby.h"
#include "UI/Lobby/WindowBase/WindowBase.h"
#include "UI/Lobby/WindowBase/BTN_WindowTab.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UUI_MainLobby::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 최초 normal pos 세팅
	const FVector2D title_bar_space = FVector2D(24.0f, 24.0f);
	FVector2D normal_pos = FVector2D(120.0f, 120.0f);
	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsValid(window_widget))
		{
			window_widget->SetLastNormalPos(FVector2D(normal_pos));
			normal_pos += title_bar_space;
		}
	}
}

void UUI_MainLobby::CreateWindow(EWindowWidgetType _type)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr))
		return;

	auto& window_widget = data_ptr->WindowWidget;
	auto& window_tab = data_ptr->WindowTab;

	if (IsAllValid(window_widget, window_tab))
		return;

	if (IsAnyInvalid(data_ptr->WindowWidgetClass, _WindowTabClass))
	{
		TRACE_ERROR(TEXT("class setting 오류."))
		return;
	}

	// window widget
	window_widget = CreateWidget<UWindowBase>(this, data_ptr->WindowWidgetClass);
	if (IsInvalid(window_widget))
	{
		TRACE_ERROR(TEXT("window widget 생성 실패."))
		return;
	}

	window_widget->_WindowWidgetType = _type;
	window_widget->SetMaximize(false);

	// set widget last normal pos
	FVector2D widget_pos = FVector2D(160.0f); // 적당한 임의의 값
	auto top_window = GetTopWindow();
	if (IsValid(top_window))
	{
		if (top_window->IsMaximized() == false)
		{
			auto top_window_slot = Cast<UCanvasPanelSlot>(top_window->Slot);
			if (IsValid(top_window_slot))
			{
				widget_pos = top_window_slot->GetPosition() + FVector2D(24.0f, 24.0f);
			}
		}
	}
	window_widget->SetLastNormalPos(widget_pos);

	window_widget->_OnWindowFocusedEvent.AddDynamic(this, &UUI_MainLobby::OnWindowFocused);
	window_widget->Hide(EWidgetHideType::Collapsed);

	auto cp_slot = CP_Window->AddChildToCanvas(window_widget);
	if (IsValid(cp_slot))
	{
		cp_slot->SetAutoSize(false);
		cp_slot->SetAlignment(FVector2D::ZeroVector);
		cp_slot->SetSize(FVector2D(600.0f));
	}

	// window tab
	if (data_ptr->CreateTab)
	{
		window_tab = CreateWidget<UBTN_WindowTab>(this, _WindowTabClass);
		if (IsInvalid(window_tab))
		{
			TRACE_ERROR(TEXT("window tab 생성 실패."))
				return;
		}

		window_tab->_WindowWidgetType = _type;
		window_tab->SetTabIcon(data_ptr->WindowTabIcon);
		window_tab->SetTabText(data_ptr->WindowTabText);

		window_tab->_OnClicked.AddDynamic(this, &UUI_MainLobby::OnClickWindowTab);

		HB_WindowTab->AddChildToHorizontalBox(window_tab);
	}
}

void UUI_MainLobby::OpenWindow(EWindowWidgetType _type, bool _is_open)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr))
		return;

	auto window_widget = data_ptr->WindowWidget;

	if (IsInvalid(window_widget))
		return;

	if (_is_open)
	{
		window_widget->Show(EWidgetShowType::SelfHitTestInvisible);

		if (!data_ptr->HasBeenOpened && data_ptr->IsMaximized)
		{
			window_widget->SetMaximize(true);
		}
		data_ptr->HasBeenOpened = true;

		SetTopWindow(window_widget);
	}
	else
	{
		window_widget->Hide(EWidgetHideType::Collapsed);
		UpdateTopWindow();
	}
}

void UUI_MainLobby::SetTopWindow(UWindowBase* _target_window)
{
	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsInvalid(cp_slot))
			continue;

		const bool is_top = window_widget == _target_window;

		cp_slot->SetZOrder(is_top ? 1 : 0);

		auto data_ptr = _WindowDataMap.Find(window_widget->_WindowWidgetType);
		if (IsValid(data_ptr))
		{
			if (IsValid(data_ptr->WindowTab))
			{
				data_ptr->WindowTab->SetHighlight(is_top);
			}
		}
	}
}

void UUI_MainLobby::UpdateTopWindow()
{
	auto top_window = GetTopWindow();
	if (IsValid(top_window))
	{
		if (top_window->IsVisible())
			return;
	}

	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		if (window_widget->IsVisible() == false)
			continue;

		SetTopWindow(window_widget);
		return;
	}
}

UWindowBase* UUI_MainLobby::GetTopWindow() const
{
	UWindowBase* top_window = nullptr;
	int32 top_z_order = -1;

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
			top_window = window_widget;
			top_z_order = z_order;
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
	if (IsInvalid(window_data_ptr))
		return;

	auto window_widget = window_data_ptr->WindowWidget;
	if (IsInvalid(window_widget))
		return;

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
