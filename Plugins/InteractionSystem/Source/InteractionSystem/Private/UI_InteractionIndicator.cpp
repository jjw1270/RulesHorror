// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_InteractionIndicator.h"
#include "CommonUtils.h"
#include "InteractableInterface.h"

void UUI_InteractionIndicator::SetInteractionActor(AActor* _interaction_actor, EInteractionState _state)
{
	ClearWidget();

	if (IsInvalid(_interaction_actor))
		return;

	_InteractionActor = _interaction_actor;
	SetActorState(_state);

	SetDisplayName(IInteractableInterface::Execute_GetDisplayName(_InteractionActor));
}

void UUI_InteractionIndicator::ClearWidget()
{
	_InteractionActor = nullptr;
	SetActorState(EInteractionState::None);
	SetDisplayName(FText::GetEmpty());
}

void UUI_InteractionIndicator::SetActorState_Implementation(EInteractionState _state)
{
	if (_ActorState == _state)
		return;

	_ActorState = _state;

	switch (_ActorState)
	{
	case EInteractionState::None:
		Hide(EWidgetHideType::Collapsed, true);
		break;

	case EInteractionState::Detected:
		Show(EWidgetShowType::SelfHitTestInvisible);
		break;

	case EInteractionState::Targeted:
		Show(EWidgetShowType::SelfHitTestInvisible);
		break;

	default:
		break;
	}
}
