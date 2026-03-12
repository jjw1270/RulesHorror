// Fill out your copyright notice in the Description page of Project Settings.


#include "BTN_WindowScaler.h"
#include "Components/Spacer.h"

void UBTN_WindowScaler::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (IsValid(Spacer))
	{
		Spacer->SetSize(_SpacerSize);
	}
}

void UBTN_WindowScaler::OnButtonStateChanged_Implementation()
{
	Super::OnButtonStateChanged_Implementation();

	UpdateCursor(_ButtonState == EButtonState::Hovered || _ButtonState == EButtonState::Pressed);
}

void UBTN_WindowScaler::UpdateCursor(bool _show_scaler_cursor)
{
	if (_show_scaler_cursor == false)
	{
		SetCursor(EMouseCursor::Default);
		return;
	}

	switch (_WindowCommandType)
	{
	case EWindowCommand::Resize_North:
	case EWindowCommand::Resize_South:
		SetCursor(EMouseCursor::ResizeUpDown);
		break;

	case EWindowCommand::Resize_West:
	case EWindowCommand::Resize_East:
		SetCursor(EMouseCursor::ResizeLeftRight);
		break;

	case EWindowCommand::Resize_NorthEast:
	case EWindowCommand::Resize_SouthWest:
		SetCursor(EMouseCursor::ResizeSouthWest);
		break;

	case EWindowCommand::Resize_NorthWest:
	case EWindowCommand::Resize_SouthEast:
		SetCursor(EMouseCursor::ResizeSouthEast);
		break;

	default:
		break;
	}
}
