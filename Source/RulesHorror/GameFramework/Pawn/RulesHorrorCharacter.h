// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "RulesHorrorCharacter.generated.h"

class UAnimSequenceBase;
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
	TObjectPtr<class UInteractorComponent> InteractorComponent = nullptr;

protected:
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> _IA_Move = nullptr;

	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> _IA_Look = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> _IA_Interact = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> _IA_ClickInteract = nullptr;

protected:
	UPROPERTY(EditAnywhere, Category = "Walk")
	float _WalkSpeed = 250.0f;

protected:
	/** 1인칭 팔 애니 (UFirstPersonArmsAnimInstance가 이동 속도에 따라 블렌드) */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimSequenceBase> _FirstPersonIdleAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimSequenceBase> _FirstPersonWalkAnim = nullptr;

public:
	ARulesHorrorCharacter();

protected:
	UFUNCTION()
	void Input_Move(const FInputActionValue& _value);

	UFUNCTION()
	void Input_Look(const FInputActionValue& _value);

	UFUNCTION()
	void Input_Interact(const FInputActionValue& _value);

	UFUNCTION()
	void Input_ClickInteract(const FInputActionValue& _value);

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
	UInteractorComponent* GetInteractorComponent() const { return InteractorComponent; }

	UAnimSequenceBase* GetFirstPersonIdleAnim() const { return _FirstPersonIdleAnim; }
	UAnimSequenceBase* GetFirstPersonWalkAnim() const { return _FirstPersonWalkAnim; }
};
