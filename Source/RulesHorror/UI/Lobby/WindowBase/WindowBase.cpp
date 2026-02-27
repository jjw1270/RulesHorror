// Fill out your copyright notice in the Description page of Project Settings.


#include "WindowBase.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Lobby/WindowBase/UI_WindowLayout.h"

void UWindowBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(WindowLayout))
	{
		WindowLayout->_OnTitleBarDoubleClickedEvent.AddDynamic(this, &UWindowBase::OnTitleBarDoubleClicked);
	}
}

void UWindowBase::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
}

void UWindowBase::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	SetMaximize(_IsMaximized);
}

FReply UWindowBase::NativeOnFocusReceived(const FGeometry& _geo, const FFocusEvent& _focus_event)
{
	if (_OnWindowFocusedEvent.IsBound())
	{
		_OnWindowFocusedEvent.Broadcast(this, true);
	}

	return Super::NativeOnFocusReceived(_geo, _focus_event);
}

void UWindowBase::NativeOnFocusLost(const FFocusEvent& _focus_event)
{
	if (_OnWindowFocusedEvent.IsBound())
	{
		_OnWindowFocusedEvent.Broadcast(this, false);
	}

	Super::NativeOnFocusLost(_focus_event);
}

FReply UWindowBase::NativeOnPreviewMouseButtonDown(const FGeometry& _geo, const FPointerEvent& _mouse_event)
{
	Super::NativeOnPreviewMouseButtonDown(_geo, _mouse_event);

	SetFocus();

	return FReply::Unhandled();
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
	const auto& anchors = cp_slot->GetAnchors();
	if (_IsMaximized)
	{
		if (anchors == maximize_anchors)
			return;
	}
	else
	{
		if (anchors == normal_anchors)
			return;
	}

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

void UWindowBase::OnTitleBarDoubleClicked()
{
	SetMaximize(!_IsMaximized);
}
