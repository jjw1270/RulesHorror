// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "LobbyPawn.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FD_OnMoveToPointFinished, const FName&, _point_name);

class UInputAction;

UCLASS(abstract)
class RULESHORROR_API ALobbyPawn : public APawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class USceneComponent> Root = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> _InputMappingContext = nullptr;

public:
	ALobbyPawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* _input_component) override;
	virtual void Tick(float _delta_time) override;

#pragma region Camera
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera = nullptr;

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
	UPROPERTY(EditAnywhere, Category = "MovePoints")
	float _MoveSpeed = 5.0f;

	UPROPERTY()
	TMap<FName, TObjectPtr<class ALobbyPawnMovePoint>> _MovePoints;

	UPROPERTY()
	TObjectPtr<ALobbyPawnMovePoint> _TargetMovePoint = nullptr;

	FD_OnMoveToPointFinished _OnMoveToPointFinishedEvent;

protected:
	void InitMovePoints();
	void DriveMoveToPoint(float _delta_time);

public:
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "_on_move_finished"))
	void SetTargetMovePoint(const FName _point_name, const FD_OnMoveToPointFinished& _on_move_finished);

#pragma endregion MovePoints
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Computer
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> _IA_LeftMouseClick = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> _IA_MouseWheel = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UWidgetInteractionComponent> WidgetInteractionComponent = nullptr;

	UPROPERTY()
	TObjectPtr<class AComputer> _InteractingComputer = nullptr;

	UPROPERTY()
	TObjectPtr<class UUI_Monitor> _InteractingMonitorWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Monitor")
	TEnumAsByte<EObjectTypeQuery> _MonitorScreenWidgetObjectType;

	// 보정 강도
	UPROPERTY(EditAnywhere, Category = "Monitor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D _MonitorHitCorrectionStrength = FVector2D(0.1f, 0.1f);

protected:
	UFUNCTION()
	void Input_LeftMouseButtonStarted();

	UFUNCTION()
	void Input_LeftMouseButtonCompleted();

	UFUNCTION()
	void Input_MouseWheel(const FInputActionValue& _value);

	void DriveWidgetInteraction(float _delta_time);
	void BuildCorrectedMonitorWidgetHit(FHitResult& _out_hit) const;
	
public:
	UFUNCTION(BlueprintCallable)
	void SetInteractingComputer(AComputer* _computer);

	UFUNCTION(BlueprintPure)
	bool IsOnInteracintingComputer() const;

#pragma endregion Computer

};
