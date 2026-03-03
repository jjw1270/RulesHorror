// Fill out your copyright notice in the Description page of Project Settings.


#include "WindowBase.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Lobby/WindowBase/UI_WindowLayout.h"
#include "UI/Lobby/WindowBase/WindowDragDropOperation.h"

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

FReply UWindowBase::NativeOnMouseButtonDown(const FGeometry& _geo, const FPointerEvent& _mouse_event)
{
	Super::NativeOnMouseButtonDown(_geo, _mouse_event);

	if (_DragType != EWindowDragType::NA)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return FReply::Unhandled();
}

void UWindowBase::NativeOnDragDetected(const FGeometry& _geo, const FPointerEvent& _mouse_event, UDragDropOperation*& _out_operation)
{
	Super::NativeOnDragDetected(_geo, _mouse_event, _out_operation);

	auto drag_operation = NewObject<UWindowDragDropOperation>();
	if (IsValid(drag_operation))
	{
		if (_DragType == EWindowDragType::Move)
		{
			if (_IsMaximized)
			{
				SetMaximize(false);
			}
		}

		drag_operation->_DragType = _DragType;

		drag_operation->Pivot = EDragPivot::MouseDown;
		drag_operation->Payload = this;
		drag_operation->_LocalOffset = _geo.AbsoluteToLocal(_mouse_event.GetScreenSpacePosition());

		_out_operation = drag_operation;
	}
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

	case EWindowCommand::Resize_UpDown:
		_DragType = EWindowDragType::Resize_UpDown;
		break;

	case EWindowCommand::Resize_LeftRight:
		_DragType = EWindowDragType::Resize_LeftRight;
		break;

	case EWindowCommand::Resize_SouthEast:
		_DragType = EWindowDragType::Resize_SouthEast;
		break;

	case EWindowCommand::Resize_SouthWest:
		_DragType = EWindowDragType::Resize_SouthWest;
		break;

	case EWindowCommand::EndDrag:
		_DragType = EWindowDragType::NA;
		break;

	default:
		break;
	}
}

const FAnchors maximize_anchors = FAnchors(0.0f, 0.0f, 1.0f, 1.0f);
const FAnchors normal_anchors = FAnchors(0.0f);

void UWindowBase::SetMaximize(bool _is_maximized)
{
	_IsMaximized = _is_maximized;

	auto cp_slot = Cast<UCanvasPanelSlot>(Slot);
	if (IsInvalid(cp_slot))
		return;

	// 중복 체크
	/*const auto& anchors = cp_slot->GetAnchors();
	if (_IsMaximized)
	{
		if (anchors == maximize_anchors)
			return;
	}
	else
	{
		if (anchors == normal_anchors)
			return;
	}*/

	if (_IsMaximized)
	{
		_LastNormalPos = cp_slot->GetPosition();

		cp_slot->SetAnchors(maximize_anchors);
		cp_slot->SetAlignment(FVector2D::ZeroVector);

		cp_slot->SetOffsets(FMargin(0.0f));
	}
	else
	{
		cp_slot->SetAnchors(normal_anchors);
		cp_slot->SetAlignment(FVector2D::ZeroVector);

		cp_slot->SetPosition(_LastNormalPos);
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
