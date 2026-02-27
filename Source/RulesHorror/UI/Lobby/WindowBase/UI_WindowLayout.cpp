// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_WindowLayout.h"
#include "Components/ClickButton.h"

void UUI_WindowLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(BTN_TitleBar))
	{
		BTN_TitleBar->_OnDoubleClicked.AddDynamic(this, &UUI_WindowLayout::OnTitleBarButtonDoubleClicked);
	}
}

void UUI_WindowLayout::OnTitleBarButtonDoubleClicked(UClickButton* _btn)
{
	if (_OnTitleBarDoubleClickedEvent.IsBound())
	{
		_OnTitleBarDoubleClickedEvent.Broadcast();
	}
}
