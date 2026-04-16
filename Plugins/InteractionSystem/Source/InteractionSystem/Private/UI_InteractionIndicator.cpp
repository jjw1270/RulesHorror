// Copyright (c) 2026 장윤제. All rights reserved.


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
}

void UUI_InteractionIndicator::ClearWidget()
{
	_InteractionActor = nullptr;
	SetActorState(EInteractionState::None);
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
		Hide(EWidgetHideType::Collapsed, false);
		break;

	default:
		break;
	}
}
