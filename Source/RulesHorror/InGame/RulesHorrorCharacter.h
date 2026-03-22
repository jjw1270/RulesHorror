// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "RulesHorrorCharacter.generated.h"

class UInputAction;

/*
*
 */
UCLASS(abstract)
class ARulesHorrorCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<class USkeletalMeshComponent> FirstPersonMesh = nullptr;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<class UCameraComponent> FirstPersonCameraComponent = nullptr;

	/** Player light source */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USpotLightComponent> SpotLight = nullptr;

	/** Interaction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UInteractionComponent> Interaction = nullptr;

protected:
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* _IA_Move = nullptr;

	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* _IA_Look = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* _IA_Interact = nullptr;

protected:
	UPROPERTY(EditAnywhere, Category = "Walk")
	float _WalkSpeed = 250.0f;

public:
	ARulesHorrorCharacter();

protected:
	void MoveInput(const FInputActionValue& _value);
	void LookInput(const FInputActionValue& _value);
	void InteractInput(const FInputActionValue& _value);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float _right, float _forward);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float _yaw, float _pitch);

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* _input_component) override;

public:
	UFUNCTION(BlueprintPure)
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	UFUNCTION(BlueprintPure)
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintPure)
	UInteractionComponent* GetInteractionComponent() const { return Interaction; }
};
