// Fill out your copyright notice in the Description page of Project Settings.


#include "Page_MainLobby.h"
#include "UI/Lobby/Explorer/Window_Explorer.h"
#include "UI/Lobby/ControlPanel/Window_ControlPanel.h"
#include "Components/CanvasPanelSlot.h"

void UPage_MainLobby::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(Window_Explorer))
	{
		Window_Explorer->_OnWindowFocusedEvent.AddDynamic(this, &UPage_MainLobby::OnWindowFocused);
	}

	if (IsValid(Window_ControlPanel))
	{
		Window_ControlPanel->_OnWindowFocusedEvent.AddDynamic(this, &UPage_MainLobby::OnWindowFocused);
	}
}

void UPage_MainLobby::NativeConstruct()
{
	Super::NativeConstruct();

}

void UPage_MainLobby::ToggleWindowExplorer()
{

}

void UPage_MainLobby::ToggleWindowControlPanel()
{

}

void UPage_MainLobby::OnWindowFocused(UWindowBase* _window_widget, bool _is_focused)
{
	if (IsInvalid(_window_widget))
		return;

	auto cp_slot = Cast<UCanvasPanelSlot>(_window_widget->Slot);
	if (IsInvalid(cp_slot))
		return;

	cp_slot->SetZOrder(_is_focused ? 1 : 0);
}
