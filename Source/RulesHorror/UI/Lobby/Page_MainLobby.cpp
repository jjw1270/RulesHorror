// Fill out your copyright notice in the Description page of Project Settings.


#include "Page_MainLobby.h"
#include "UI/Lobby/WindowBase/WindowBase.h"
#include "UI/Lobby/WindowBase/BTN_WindowTab.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/Lobby/WindowBase/WindowDragDropOperation.h"

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

	data_ptr->WindowWidget->_OnWindowFocusedEvent.AddDynamic(this, &UPage_MainLobby::OnWindowFocused);
	data_ptr->WindowWidget->Hide(EWidgetHideType::Collapsed);

	auto cp_slot = CP_Window->AddChildToCanvas(data_ptr->WindowWidget);
	if (IsValid(cp_slot))
	{
		cp_slot->SetAutoSize(false);
		cp_slot->SetAlignment(FVector2D::ZeroVector);
		cp_slot->SetSize(FVector2D(600.0f));
	}

	// window tab
	if (data_ptr->CreateTab)
	{
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
}

void UPage_MainLobby::OpenWindow(EWindowWidgetType _type, bool _is_open)
{
	auto data_ptr = _WindowDataMap.Find(_type);
	if (IsInvalid(data_ptr))
		return;

	if (IsInvalid(data_ptr->WindowWidget))
		return;

	if (_is_open)
	{
		data_ptr->WindowWidget->Show(EWidgetShowType::SelfHitTestInvisible);

		if (data_ptr->IsMaximized)
		{
			data_ptr->WindowWidget->SetMaximize(true);
		}

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

bool UPage_MainLobby::NativeOnDragOver(const FGeometry& _geo, const FDragDropEvent& _drag_drop_event, UDragDropOperation* _operation)
{
	Super::NativeOnDragOver(_geo, _drag_drop_event, _operation);

	auto drag_operation = Cast<UWindowDragDropOperation>(_operation);
	if (IsInvalid(drag_operation))
		return false;

	auto window_widget = Cast<UWindowBase>(drag_operation->Payload);
	if (IsInvalid(window_widget))
		return false;

	if (drag_operation->_DragType == EWindowDragType::Move)
	{
		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsInvalid(cp_slot))
			return false;

		const FVector2D canvas_local = _geo.AbsoluteToLocal(_drag_drop_event.GetScreenSpacePosition());
		const FVector2D new_pos = canvas_local - drag_operation->_LocalOffset;

		cp_slot->SetPosition(new_pos);
	}
	else
	{
		const FVector2D drag_delta = _drag_drop_event.GetScreenSpacePosition() - drag_operation->_DragStartScreenPos;

		window_widget->ResizeWindow(drag_operation->_DragType, drag_operation->_InitialWindowPos, drag_operation->_InitialWindowSize, drag_delta);
	}

	return true;
}

bool UPage_MainLobby::NativeOnDrop(const FGeometry& _geo, const FDragDropEvent& _drag_drop_event, UDragDropOperation* _operation)
{
	Super::NativeOnDragOver(_geo, _drag_drop_event, _operation);

	auto drag_operation = Cast<UWindowDragDropOperation>(_operation);
	if (IsInvalid(drag_operation))
		return false;

	auto window_widget = Cast<UWindowBase>(drag_operation->Payload);
	if (IsInvalid(window_widget))
		return false;

	if (drag_operation->_DragType == EWindowDragType::Move)
	{
		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsInvalid(cp_slot))
			return false;

		const FVector2D canvas_local = _geo.AbsoluteToLocal(_drag_drop_event.GetScreenSpacePosition());
		const FVector2D new_pos = canvas_local - drag_operation->_LocalOffset;

		cp_slot->SetPosition(new_pos);
	}
	else
	{
		const FVector2D drag_delta = _drag_drop_event.GetScreenSpacePosition() - drag_operation->_DragStartScreenPos;

		window_widget->ResizeWindow(drag_operation->_DragType, drag_operation->_InitialWindowPos, drag_operation->_InitialWindowSize, drag_delta);
	}

	return true;
}

void UPage_MainLobby::OnClickWindowTab(UButtonBase* _tab_button)
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

void UPage_MainLobby::OnWindowFocused(UWindowBase* _focused_window_widget, bool _is_focused)
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
