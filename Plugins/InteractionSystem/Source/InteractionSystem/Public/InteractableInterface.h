// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionSystemDefines.h"
#include "CommonUtils.h"
#include "InteractableInterface.generated.h"

UINTERFACE(BlueprintType)
class INTERACTIONSYSTEM_API UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class INTERACTIONSYSTEM_API IInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction))
	void SetInteractionState(EInteractionState _state);
	virtual void SetInteractionState_Implementation(EInteractionState _state)
	{
		TRACE_LOG(TEXT("%s"), *TEnumToString(_state));
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction, Tooltip = "감지 가능한지"))
	bool CanBeDetected() const;
	virtual bool CanBeDetected_Implementation() const 
	{
		return true;
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction, Tooltip = "상호작용 가능한지"))
	bool CanStartInteract() const;
	virtual bool CanStartInteract_Implementation() const
	{
		return true;
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction))
	void StartInteract(AActor* _interactor);
	virtual void StartInteract_Implementation(AActor* _interactor)
	{
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction))
	void EndInteract(AActor* _interactor);
	virtual void EndInteract_Implementation(AActor* _interactor)
	{
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction))
	FText GetDisplayName() const;
	virtual FText GetDisplayName_Implementation() const
	{
		return FText::FromString(TEXT("Interact"));
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction))
	FVector GetInteractionLocation() const;
	virtual FVector GetInteractionLocation_Implementation() const
	{
		const auto actor = Cast<AActor>(_getUObject());
		return IsValid(actor) ? actor->GetActorLocation() : FVector::ZeroVector;
	}

};
