// Fill out your copyright notice in the Description page of Project Settings.


#include "Page_MainLobby.h"
#include "UI/Lobby/WindowBase/WindowBase.h"
#include "UI/Lobby/WindowBase/BTN_WindowTab.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UPage_MainLobby::NativeOnInitialized()
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

void UPage_MainLobby::CreateWindow(EWindowWidgetType _type)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr))
		return;

	if (IsAllValid(data_ptr->WindowWidget, data_ptr->WindowTab))
		return;

	if (IsAnyInvalid(data_ptr->WindowWidgetClass, _WindowTabClass))
	{
		TRACE_ERROR(TEXT("class setting 오류."))
		return;
	}

	// window widget
	data_ptr->WindowWidget = CreateWidget<UWindowBase>(this, data_ptr->WindowWidgetClass);
	if (IsInvalid(data_ptr->WindowWidget))
	{
		TRACE_ERROR(TEXT("window widget 생성 실패."))
		return;
	}

	data_ptr->WindowWidget->_WindowWidgetType = _type;
	data_ptr->WindowWidget->SetMaximize(false);

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
	data_ptr->WindowWidget->SetLastNormalPos(widget_pos);

	auto cp_slot = CP_Window->AddChildToCanvas(data_ptr->WindowWidget);
	if (IsValid(cp_slot))
	{
		cp_slot->SetAutoSize(true);
	}

	// window tab
	data_ptr->WindowTab = CreateWidget<UBTN_WindowTab>(this, _WindowTabClass);
	if (IsInvalid(data_ptr->WindowTab))
	{
		TRACE_ERROR(TEXT("window tab 생성 실패."))
		return;
	}

	data_ptr->WindowTab->_WindowWidgetType = _type;
	data_ptr->WindowTab->SetTabIcon(data_ptr->WindowTabIcon);
	data_ptr->WindowTab->SetTabText(data_ptr->WindowTabText);

	data_ptr->WindowTab->_OnClicked.AddDynamic(this, &UPage_MainLobby::OnClickWindowTab);

	HB_WindowTab->AddChildToHorizontalBox(data_ptr->WindowTab);
}

void UPage_MainLobby::OpenWindow(EWindowWidgetType _type, bool _is_open)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr))
		return;

	if (IsAnyInvalid(data_ptr->WindowWidget, data_ptr->WindowTab))
		return;

	if (_is_open)
	{
		data_ptr->WindowWidget->Show(EWidgetShowType::SelfHitTestInvisible);
		SetTopWindow(data_ptr->WindowWidget);
	}
	else
	{
		data_ptr->WindowWidget->Hide(EWidgetHideType::Collapsed);
		UpdateTopWindow();
	}
}

void UPage_MainLobby::SetTopWindow(UWindowBase* _target_window)
{
	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsInvalid(cp_slot))
			continue;

		cp_slot->SetZOrder(window_widget == _target_window ? 1 : 0);
	}
}

void UPage_MainLobby::UpdateTopWindow()
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

UWindowBase* UPage_MainLobby::GetTopWindow() const
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

void UPage_MainLobby::OnClickWindowTab(UButtonBase* _tab_button)
{
	/*if (IsInvalid(_tab_button))
		return;

	UWindowBase* window_widget = nullptr;

	if (_tab_button == BTN_ExplorerTab)
	{
		window_widget = Window_Explorer;
	}
	else if (_tab_button == BTN_ControlPanelTab)
	{
		window_widget = Window_ControlPanel;
	}

	if (IsInvalid(window_widget))
		return;

	switch (window_widget->GetWidgetState())
	{
	case EWidgetState::Hiding:
	case EWidgetState::Hide:
		window_widget->Show(EWidgetShowType::SelfHitTestInvisible);
		window_widget->SetWindowFocused(true);
		break;

	case EWidgetState::Showing:
	case EWidgetState::Idle:
	{
		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsValid(cp_slot))
		{
			if (cp_slot->GetZOrder() == 0)
			{
				window_widget->SetWindowFocused(true);
			}
			else
			{
				window_widget->Hide(EWidgetHideType::Collapsed);
			}
		}
	}
		break;

	default:
		break;
	}*/
}

void UPage_MainLobby::OnWindowFocused(UWindowBase* _focused_window_widget, bool _is_focused)
{
	/*if (_is_focused == false)
		return;

	if (IsInvalid(_focused_window_widget))
		return;

	for (auto child : CP_Window->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsValid(cp_slot))
		{
			cp_slot->SetZOrder(_focused_window_widget == window_widget ? 1 : 0);
		}
	}*/
}
