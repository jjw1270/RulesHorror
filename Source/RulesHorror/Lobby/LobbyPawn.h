// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "LobbyPawn.generated.h"

UCLASS(abstract)
class RULESHORROR_API ALobbyPawn : public APawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class USceneComponent> Root = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UInteractionComponent> InteractionComponent = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> _InputMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> _IA_Interact = nullptr;

protected:
	bool _CanInteract = true;

	bool _CanLook = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float _MouseSensitivity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float _FollowSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float _DeadZoneNormalized = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector2D _YawRange = FVector2D(-30.0f, 30.0f);

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector2D _PitchRange = FVector2D(-10.0f, 10.0f);

	float _CurrentYaw = 0.0f;
	float _TargetYaw = 0.0f;

	float _CurrentPitch = 0.0f;
	float _TargetPitch = 0.0f;

public:
	ALobbyPawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* _input_component) override;
	virtual void Tick(float _delta_time) override;

protected:
	void InteractInput(const FInputActionValue& _value);

	void UpdateLookTargetFromMouse();

public:
	void SetInteractEnabled(bool _is_enabled);
	
	UFUNCTION(BlueprintCallable)
	void SetLookEnabled(bool _is_enabled);

	UFUNCTION(BlueprintPure)
	UCameraComponent* GetLobbyCamera() const { return Camera; }

	UFUNCTION(BlueprintPure)
	UInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

};
