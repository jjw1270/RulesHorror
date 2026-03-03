// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_WindowLayout.h"
#include "Components/SizeBox.h"


void UUI_WindowLayout::RequestCommand(EWindowCommand _command)
{
	_OnRequestCommandEvent.ExecuteIfBound(_command);
}

void UUI_WindowLayout::SetSize(const FVector2D& _size)
{
	SizeBox->SetWidthOverride(_size.X);
	SizeBox->SetHeightOverride(_size.Y);
}
