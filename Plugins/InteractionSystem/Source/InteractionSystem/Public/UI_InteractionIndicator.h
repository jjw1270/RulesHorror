// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "InteractionSystemDefines.h"
#include "UI_InteractionIndicator.generated.h"

/**
 * 
 */
UCLASS(abstract)
class INTERACTIONSYSTEM_API UUI_InteractionIndicator : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> _InteractionActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	EInteractionState _ActorState = EInteractionState::None;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetDetectedIconSize(const FVector2D& _size);

	void SetInteractionActor(AActor* _interaction_actor, EInteractionState _state);
	void ClearWidget();

	UFUNCTION(BlueprintNativeEvent)
	void SetActorState(EInteractionState _state);
	void SetActorState_Implementation(EInteractionState _state);

	UFUNCTION(BlueprintPure)
	EInteractionState GetActorState() const { return _ActorState; }

};
