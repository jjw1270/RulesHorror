// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_WindowLayout.h"
#include "Components/ClickButton.h"


void UUI_WindowLayout::RequestCommand(EWindowCommand _command)
{
	_OnRequestCommandEvent.ExecuteIfBound(_command);
}
