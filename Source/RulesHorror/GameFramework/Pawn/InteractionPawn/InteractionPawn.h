// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "InteractionSystemDefines.h"
#include "InteractionPawn.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FD_OnMoveToPointFinished, const FName&, _point_name);

class UInputAction;

UCLASS(abstract)
class RULESHORROR_API AInteractionPawn : public APawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class USceneComponent> Root = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> _InputMappingContext = nullptr;

public:
	AInteractionPawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* _input_component) override;
	virtual void Tick(float _delta_time) override;

#pragma region Camera
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	bool _EnableLookAtCursor = true;

	bool _UseLookAtCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float _MouseSensitivity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float _FollowSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float _DeadZoneNormalizedYaw = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float _DeadZoneNormalizedPitch = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector2D _YawRange = FVector2D(-30.0f, 30.0f);

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector2D _PitchRange = FVector2D(-10.0f, 10.0f);

	float _BaseYaw = 0.0f;
	float _CurrentYaw = 0.0f;
	float _TargetYaw = 0.0f;

	float _BasePitch = 0.0f;
	float _CurrentPitch = 0.0f;
	float _TargetPitch = 0.0f;

protected:
	void UpdateLookAtCursor();

public:
	UFUNCTION(BlueprintCallable)
	void SetUseLookAtCursor(bool _value);

	UFUNCTION(BlueprintPure)
	UCameraComponent* GetCamera() const { return Camera; }

#pragma endregion Camera
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Interation
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UInteractorComponent> InteractorComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> _IA_Interact = nullptr;

	bool _CanInteract = true;

	EInteractionDetectMode _DefaultDetectMode = EInteractionDetectMode::NA;

protected:
	UFUNCTION()
	void Input_Interact(const FInputActionValue& _value);

public:
	UFUNCTION(BlueprintCallable)
	void SetInteractEnabled(bool _is_enabled);

	UFUNCTION(BlueprintPure)
	UInteractorComponent* GetInteractorComponent() const { return InteractorComponent; }

#pragma endregion Interation
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region MovePoints
protected:
	UPROPERTY(EditAnywhere, Category = "MovePoint", meta = (Tooltip = "cm/s"))
	float _MoveSpeed = 200.0f;

	UPROPERTY(EditAnywhere, Category = "MovePoint", meta = (Tooltip = "deg/s"))
	float _RotateSpeed = 100.0f;

	UPROPERTY(EditAnywhere, Category = "MovePoint")
	FName _StartMovePoint;

	FVector _MoveStartLocation;
	FRotator _MoveStartRotation;

	float _MoveElapsedTime = 0.0f;
	float _MoveDuration = 0.0f;

	UPROPERTY()
	TMap<FName, TObjectPtr<class AInteractionPawnMovePoint>> _MovePoints;

	UPROPERTY()
	TObjectPtr<AInteractionPawnMovePoint> _TargetMovePoint = nullptr;

	FD_OnMoveToPointFinished _OnMoveToPointFinishedEvent;

protected:
	void InitMovePoints();
	void DriveMoveToPoint(float _delta_time);

public:
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "_on_move_finished"))
	void SetTargetMovePoint(const FName _point_name, bool _is_teleport, const FD_OnMoveToPointFinished& _on_move_finished);

#pragma endregion MovePoints
};
