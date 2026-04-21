// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionSystemDefines.h"
#include "CommonUtils.h"
#include "Components/MeshComponent.h"
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
	virtual void SetInteractionState_Implementation(EInteractionState _state) {};

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction, Tooltip = "감지 가능한지"))
	bool CanBeDetected() const;
	virtual bool CanBeDetected_Implementation() const 
	{
		return true;
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction, Tooltip = "자기 자신이 가릴 수 있는지"))
	bool CanBeCollapsedBySelf() const;
	virtual bool CanBeCollapsedBySelf_Implementation() const
	{
		return false;
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction, Tooltip = "상호작용 가능한지"))
	bool CanInteract() const;
	virtual bool CanInteract_Implementation() const
	{
		return true;
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction))
	void Interact(AActor* _interactor);
	virtual void Interact_Implementation(AActor* _interactor) {};

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

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction", meta = (ForceAsFunction))
	TSet<UMeshComponent*> GetEffectedMeshComponents() const;
	virtual TSet<UMeshComponent*> GetEffectedMeshComponents_Implementation() const
	{
		// override 하지 않으면 모든 Mesh 컴포넌트를 가져옴.
		TSet<UMeshComponent*> mesh_components;

		const auto actor = Cast<AActor>(_getUObject());
		if (IsInvalid(actor))
			return mesh_components;

		for (auto comp : actor->GetComponents())
		{
			auto mesh_comp = Cast<UMeshComponent>(comp);
			if (IsValid(mesh_comp))
			{
				mesh_components.Add(mesh_comp);
			}
		}

		return mesh_components;
	}

};
