// Fill out your copyright notice in the Description page of Project Settings.


#include "WindowBase.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "UI/Lobby/WindowBase/UI_WindowLayout.h"

void UWindowBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetIsFocusable(true);

	if (IsValid(WindowLayout))
	{
		WindowLayout->_OnRequestCommandEvent.Unbind();
		WindowLayout->_OnRequestCommandEvent.BindUObject(this, &UWindowBase::ExecuteCommand);
	}
}

void UWindowBase::NativeConstruct()
{
	Super::NativeConstruct();

	SetMaximize(_IsMaximized);
}

FReply UWindowBase::NativeOnPreviewMouseButtonDown(const FGeometry& _geo, const FPointerEvent& _mouse_event)
{
	Super::NativeOnPreviewMouseButtonDown(_geo, _mouse_event);

	SetWindowFocused(true);

	return FReply::Unhandled();
}

void UWindowBase::ExecuteCommand(EWindowCommand _command)
{
	switch (_command)
	{
	case EWindowCommand::Minimize:
		Hide(EWidgetHideType::Collapsed);
		break;

	case EWindowCommand::RestoreSize:
		SetMaximize(!_IsMaximized);
		break;

	case EWindowCommand::Close:
		RemoveFromParent();
		break;

	case EWindowCommand::Move:
		_DragType = EWindowDragType::Move;
		break;

	case EWindowCommand::Resize_North:
		_DragType = EWindowDragType::Resize_North;
		break;

	case EWindowCommand::Resize_South:
		_DragType = EWindowDragType::Resize_South;
		break;

	case EWindowCommand::Resize_West:
		_DragType = EWindowDragType::Resize_West;
		break;

	case EWindowCommand::Resize_East:
		_DragType = EWindowDragType::Resize_East;
		break;

	case EWindowCommand::Resize_NorthWest:
		_DragType = EWindowDragType::Resize_NorthWest;
		break;

	case EWindowCommand::Resize_NorthEast:
		_DragType = EWindowDragType::Resize_NorthEast;
		break;

	case EWindowCommand::Resize_SouthWest:
		_DragType = EWindowDragType::Resize_SouthWest;
		break;

	case EWindowCommand::Resize_SouthEast:
		_DragType = EWindowDragType::Resize_SouthEast;
		break;

	case EWindowCommand::EndDrag:
		_DragType = EWindowDragType::NA;
		break;

	default:
		break;
	}
}

FReply UWindowBase::NativeOnMouseButtonDown(const FGeometry& _geo, const FPointerEvent& _mouse_event)
{
	Super::NativeOnMouseButtonDown(_geo, _mouse_event);

	if (_DragType == EWindowDragType::NA)
		return FReply::Unhandled();

	auto cp_slot = Cast<UCanvasPanelSlot>(Slot);
	if (IsInvalid(cp_slot))
		return FReply::Unhandled();

	_IsDragging = true;
	_DragStartCursorPos =_mouse_event.GetScreenSpacePosition();
	_InitialWindowPos = cp_slot->GetPosition();
	_InitialWindowSize = cp_slot->GetSize();

	// 클릭한 위치 비율 저장
	const FVector2D local_mouse_pos = _geo.AbsoluteToLocal(_DragStartCursorPos);
	const FVector2D local_size = _geo.GetLocalSize();
	_DragStartLocalRatio.X = (local_size.X > KINDA_SMALL_NUMBER) ? FMath::Clamp(local_mouse_pos.X / local_size.X, 0.0f, 1.0f) : 0.0f;
	_DragStartLocalRatio.Y = (local_size.Y > KINDA_SMALL_NUMBER) ? FMath::Clamp(local_mouse_pos.Y / local_size.Y, 0.0f, 1.0f) : 0.0f;

	return FReply::Handled();
}

FReply UWindowBase::NativeOnMouseButtonUp(const FGeometry& _geo, const FPointerEvent& _mouse_event)
{
	Super::NativeOnMouseButtonUp(_geo, _mouse_event);

	_IsDragging = false;
	_DragType = EWindowDragType::NA;

	return FReply::Handled();
}

void UWindowBase::UpdateDrag(const FVector2D& _current_cursor_pos)
{
	if (_IsDragging == false)
		return;

	const FVector2D drag_delta = _current_cursor_pos - _DragStartCursorPos;

	if (drag_delta.SizeSquared() < _DragDeadZone)
		return;

	if (_DragType == EWindowDragType::Move)
	{
		auto cp_slot = Cast<UCanvasPanelSlot>(Slot);
		if (IsValid(cp_slot))
		{
			if (_IsMaximized)
			{
				SetMaximize(false);

				const FVector2D restored_size = _LastNormalSize;
				const FVector2D restored_pos = _current_cursor_pos - FVector2D(restored_size.X * _DragStartLocalRatio.X, restored_size.Y * _DragStartLocalRatio.Y);

				cp_slot->SetPosition(restored_pos);
				cp_slot->SetSize(restored_size);

				_InitialWindowPos = restored_pos;
				_InitialWindowSize = restored_size;
				_DragStartCursorPos = _current_cursor_pos;

				return;
			}

			cp_slot->SetPosition(_InitialWindowPos + drag_delta);
		}
	}
	else
	{
		ResizeWindow(_DragType, _InitialWindowPos, _InitialWindowSize, drag_delta);
	}
}

void UWindowBase::ResizeWindow(EWindowDragType _drag_type, const FVector2D& _initial_window_pos, const FVector2D& _initial_window_size, const FVector2D& _drag_delta)
{
	auto cp_slot = Cast<UCanvasPanelSlot>(Slot);
	if (IsInvalid(cp_slot))
		return;

	const FVector2D viewport_size = UWidgetLayoutLibrary::GetViewportSize(this);

	const float min_width = 300.f;
	const float min_height = 200.f;

	FVector2D new_pos = _initial_window_pos;
	FVector2D new_size = _initial_window_size;

	const float initial_right = _initial_window_pos.X + _initial_window_size.X;
	const float initial_bottom = _initial_window_pos.Y + _initial_window_size.Y;

	switch (_drag_type)
	{
	case EWindowDragType::Resize_North:
		new_size.Y = FMath::Max(min_height, _initial_window_size.Y - _drag_delta.Y);
		new_pos.Y = initial_bottom - new_size.Y;
		break;

	case EWindowDragType::Resize_South:
		new_size.Y = FMath::Max(min_height, _initial_window_size.Y + _drag_delta.Y);
		break;

	case EWindowDragType::Resize_West:
		new_size.X = FMath::Max(min_width, _initial_window_size.X - _drag_delta.X);
		new_pos.X = initial_right - new_size.X;
		break;

	case EWindowDragType::Resize_East:
		new_size.X = FMath::Max(min_width, _initial_window_size.X + _drag_delta.X);
		break;

	case EWindowDragType::Resize_NorthWest:
		new_size.X = FMath::Max(min_width, _initial_window_size.X - _drag_delta.X);
		new_size.Y = FMath::Max(min_height, _initial_window_size.Y - _drag_delta.Y);
		new_pos.X = initial_right - new_size.X;
		new_pos.Y = initial_bottom - new_size.Y;
		break;

	case EWindowDragType::Resize_NorthEast:
		new_size.X = FMath::Max(min_width, _initial_window_size.X + _drag_delta.X);
		new_size.Y = FMath::Max(min_height, _initial_window_size.Y - _drag_delta.Y);
		new_pos.Y = initial_bottom - new_size.Y;
		break;

	case EWindowDragType::Resize_SouthWest:
		new_size.X = FMath::Max(min_width, _initial_window_size.X - _drag_delta.X);
		new_size.Y = FMath::Max(min_height, _initial_window_size.Y + _drag_delta.Y);
		new_pos.X = initial_right - new_size.X;
		break;

	case EWindowDragType::Resize_SouthEast:
		new_size.X = FMath::Max(min_width, _initial_window_size.X + _drag_delta.X);
		new_size.Y = FMath::Max(min_height, _initial_window_size.Y + _drag_delta.Y);
		break;

	default:
		return;
	}

	cp_slot->SetPosition(new_pos);
	cp_slot->SetSize(new_size);
}

void UWindowBase::SetMaximize(bool _is_maximized)
{
	_IsMaximized = _is_maximized;

	auto cp_slot = Cast<UCanvasPanelSlot>(Slot);
	if (IsInvalid(cp_slot))
		return;

	const FAnchors maximize_anchors = FAnchors(0.0f, 0.0f, 1.0f, 1.0f);
	const FAnchors normal_anchors = FAnchors(0.0f);

	if (_IsMaximized)
	{
		_LastNormalPos = cp_slot->GetPosition();
		_LastNormalSize = cp_slot->GetSize();

		cp_slot->SetAnchors(maximize_anchors);
		cp_slot->SetAlignment(FVector2D::ZeroVector);

		cp_slot->SetOffsets(FMargin(0.0f));
	}
	else
	{
		cp_slot->SetAnchors(normal_anchors);
		cp_slot->SetAlignment(FVector2D::ZeroVector);

		cp_slot->SetPosition(_LastNormalPos);
		cp_slot->SetSize(_LastNormalSize);
	}

	WindowLayout->EnableWindowScaler(!_IsMaximized);
}

void UWindowBase::SetWindowFocused(bool _is_focused)
{
	if (_OnWindowFocusedEvent.IsBound())
	{
		_OnWindowFocusedEvent.Broadcast(this, _is_focused);
	}

	OnWindowFocused(_is_focused);
}

void UWindowBase::OnWindowFocused(bool _is_focused)
{
	WindowLayout->OnFocused(_is_focused);

}
