// Fill out your copyright notice in the Description page of Project Settings.


#include "WindowBase.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Lobby/WindowBase/UI_WindowLayout.h"
#include "Blueprint/DragDropOperation.h"

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

	if (_ShouldDrag)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return FReply::Unhandled();
}

void UWindowBase::NativeOnDragDetected(const FGeometry& _geo, const FPointerEvent& _mouse_event, UDragDropOperation*& _out_operation)
{
	Super::NativeOnDragDetected(_geo, _mouse_event, _out_operation);

	auto drag_operation = NewObject<UDragDropOperation>();
	if (IsValid(drag_operation))
	{
		drag_operation->Pivot = EDragPivot::MouseDown;
		drag_operation->Payload = this;

		auto visual_widget = CreateWidget<UWindowBase>(this, this->GetClass());
		if(IsValid(visual_widget))
		{
			visual_widget->SetIsShowOnNextTick(false);
			visual_widget->SetRenderOpacity(0.2f);
			drag_operation->DefaultDragVisual = visual_widget;
		}

		_out_operation = drag_operation;

		Hide(EWidgetHideType::Collapsed, true);
	}
}

bool UWindowBase::NativeOnDrop(const FGeometry& _geo, const FDragDropEvent& _drag_drop_event, UDragDropOperation* _operation)
{
	Super::NativeOnDrop(_geo, _drag_drop_event, _operation);

	if (IsInvalid(_operation))
		return false;

	auto window_widget = Cast<UWindowBase>(_operation->Payload);
	if (IsValid(window_widget))
	{

	}

	return false;
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
		// hide와 동일한 기능?
		Hide(EWidgetHideType::Collapsed);
		break;
	case EWindowCommand::StartDrag:
		if(_IsMaximized)
		{
			SetMaximize(false);
		}

		_ShouldDrag = true;
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
		cp_slot->SetSize(_NormalSize);
	}
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
