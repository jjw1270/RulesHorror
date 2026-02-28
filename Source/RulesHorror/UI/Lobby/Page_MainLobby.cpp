// Fill out your copyright notice in the Description page of Project Settings.


#include "Page_MainLobby.h"
#include "UI/Lobby/Explorer/Window_Explorer.h"
#include "UI/Lobby/ControlPanel/Window_ControlPanel.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Widgets/Components/ClickButton.h"

void UPage_MainLobby::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(Window_Explorer))
	{
		Window_Explorer->_OnWindowFocusedEvent.RemoveAll(this);
		Window_Explorer->_OnWindowFocusedEvent.AddDynamic(this, &UPage_MainLobby::OnWindowFocused);
	}

	if (IsValid(BTN_ExplorerTab))
	{
		BTN_ExplorerTab->_OnClicked.RemoveAll(this);
		BTN_ExplorerTab->_OnClicked.AddDynamic(this, &UPage_MainLobby::OnClickWindowTabButton);
	}

	if (IsValid(Window_ControlPanel))
	{
		Window_ControlPanel->_OnWindowFocusedEvent.RemoveAll(this);
		Window_ControlPanel->_OnWindowFocusedEvent.AddDynamic(this, &UPage_MainLobby::OnWindowFocused);
	}

	if (IsValid(BTN_ControlPanelTab))
	{
		BTN_ControlPanelTab->_OnClicked.RemoveAll(this);
		BTN_ControlPanelTab->_OnClicked.AddDynamic(this, &UPage_MainLobby::OnClickWindowTabButton);
	}
	
	// 최초 normal pos 세팅
	const FVector2D title_bar_space = FVector2D(24.0f, 24.0f);
	FVector2D normal_pos = FVector2D(120.0f, 120.0f);
	for (auto child : CP_Windows->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsValid(window_widget))
		{
			window_widget->SetLastNormalPos(FVector2D(normal_pos));
			normal_pos += title_bar_space;
		}
	}
}

void UPage_MainLobby::OnWindowFocused(UWindowBase* _focused_window_widget, bool _is_focused)
{
	if (_is_focused == false)
		return;

	if (IsInvalid(_focused_window_widget))
		return;

	for (auto child : CP_Windows->GetAllChildren())
	{
		auto window_widget = Cast<UWindowBase>(child);
		if (IsInvalid(window_widget))
			continue;

		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsValid(cp_slot))
		{
			cp_slot->SetZOrder(_focused_window_widget == window_widget ? 1 : 0);
		}
	}
}

void UPage_MainLobby::OnClickWindowTabButton(UButtonBase* _tab_button)
{
	if (IsInvalid(_tab_button))
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
		window_widget->SetFocus();
		break;

	case EWidgetState::Showing:
	case EWidgetState::Idle:
	{
		auto cp_slot = Cast<UCanvasPanelSlot>(window_widget->Slot);
		if (IsValid(cp_slot))
		{
			if (cp_slot->GetZOrder() == 0)
			{
				OnWindowFocused(window_widget, true);
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
	}
}
