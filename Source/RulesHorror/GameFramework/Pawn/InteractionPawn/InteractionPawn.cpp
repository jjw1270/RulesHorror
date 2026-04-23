// Copyright (c) 2026 장윤제. All rights reserved.


#include "InteractionPawn.h"
#include "RulesHorrorUtils.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InteractorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn/InteractionPawn/InteractionPawnMovePoint.h"

AInteractionPawn::AInteractionPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	InteractorComponent = CreateDefaultSubobject<UInteractorComponent>(TEXT("Interactor"));
	InteractorComponent->SetupAttachment(Root);
}

void AInteractionPawn::BeginPlay()
{
	Super::BeginPlay();

	_DefaultDetectMode = InteractorComponent->GetDetectMode();

	_BaseYaw = GetActorRotation().Yaw;
	_BasePitch = GetActorRotation().Pitch;

	auto pc = Cast<APlayerController>(GetController());
	if (IsValid(pc))
	{
		pc->bShowMouseCursor = true;
		pc->bEnableClickEvents = false;
		pc->bEnableMouseOverEvents = false;

		FInputModeGameAndUI input_mode;
		input_mode.SetHideCursorDuringCapture(false);
		input_mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		pc->SetInputMode(input_mode);

		auto local_player = pc->GetLocalPlayer();
		if (IsValid(local_player))
		{
			auto enhanced_input_subsys = local_player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (IsValid(enhanced_input_subsys))
			{
				if (IsValid(_InputMappingContext))
				{
					enhanced_input_subsys->AddMappingContext(_InputMappingContext, 0);
				}
			}
		}
	}

	InitMovePoints();

	if(_StartMovePoint.IsNone() == false)
	{
		SetTargetMovePoint(_StartMovePoint, true, FD_OnMoveToPointFinished());
	}
}

void AInteractionPawn::SetupPlayerInputComponent(UInputComponent* _input_component)
{
	Super::SetupPlayerInputComponent(_input_component);

	auto enhanced_input_comp = Cast<UEnhancedInputComponent>(_input_component);
	if (IsInvalid(enhanced_input_comp))
	{
		TRACE_ERROR(TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
		return;
	}

	if (IsAnyInvalid(_IA_Interact))
	{
		TRACE_ERROR(TEXT("Some Input Action is not set!!"));
		return;
	}

	enhanced_input_comp->BindAction(_IA_Interact, ETriggerEvent::Started, this, &AInteractionPawn::Input_Interact);
}

void AInteractionPawn::Tick(float _delta_time)
{
	Super::Tick(_delta_time);

	if (_EnableLookAtCursor && _UseLookAtCursor)
	{
		UpdateLookAtCursor();

		_CurrentYaw = FMath::FInterpTo(_CurrentYaw, _TargetYaw, _delta_time, _FollowSpeed);
		_CurrentPitch = FMath::FInterpTo(_CurrentPitch, _TargetPitch, _delta_time, _FollowSpeed);

		SetActorRotation(FRotator(_BasePitch + _CurrentPitch, _BaseYaw + _CurrentYaw, 0.0f));
	}
	else
	{
		DriveMoveToPoint(_delta_time);
	}
}

void AInteractionPawn::UpdateLookAtCursor()
{
	auto pc = Cast<APlayerController>(GetController());
	if (IsInvalid(pc))
		return;

	int32 viewport_x, viewport_y;
	pc->GetViewportSize(viewport_x, viewport_y);

	if (viewport_x <= 0 || viewport_y <= 0)
		return;

	float mouse_x, mouse_y;
	if (pc->GetMousePosition(mouse_x, mouse_y) == false)
		return;

	const float center_x = viewport_x * 0.5f;
	const float center_y = viewport_y * 0.5f;

	float normalized_x = (mouse_x - center_x) / center_x;   // -1.0f ~ 1.0f
	float normalized_y = (mouse_y - center_y) / center_y;   // -1.0f ~ 1.0f

	normalized_x = FMath::Sign(normalized_x) * FMath::Square(normalized_x);
	normalized_y = FMath::Sign(normalized_y) * FMath::Square(normalized_y);

	// 데드존
	if (_DeadZoneNormalizedYaw > 0.0f && FMath::Abs(normalized_x) < _DeadZoneNormalizedYaw)
	{
		normalized_x = 0.0f;
	}

	if (_DeadZoneNormalizedPitch > 0.0f && FMath::Abs(normalized_y) < _DeadZoneNormalizedPitch)
	{
		normalized_y = 0.0f;
	}

	_TargetYaw = FMath::Lerp(_YawRange.X, _YawRange.Y, (normalized_x + 1.0f) * 0.5f);
	_TargetPitch = FMath::Lerp(_PitchRange.Y, _PitchRange.X, (normalized_y + 1.0f) * 0.5f);
}

void AInteractionPawn::SetUseLookAtCursor(bool _value)
{
	if (_UseLookAtCursor == _value)
		return;

	_UseLookAtCursor = _value;

	if (_value)
	{
		const FRotator current_rotation = GetActorRotation();

		_BaseYaw = current_rotation.Yaw;
		_BasePitch = current_rotation.Pitch;

		_CurrentYaw = 0.0f;
		_CurrentPitch = 0.0f;
		_TargetYaw = 0.0f;
		_TargetPitch = 0.0f;
	}
}

void AInteractionPawn::Input_Interact(const FInputActionValue& _value)
{
	if (_CanInteract == false)
		return;

	InteractorComponent->TryInteract();
}

void AInteractionPawn::SetInteractEnabled(bool _is_enabled)
{
	_CanInteract = _is_enabled;
}

void AInteractionPawn::InitMovePoints()
{
	_MovePoints.Empty();

	TArray<AActor*> move_points;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AInteractionPawnMovePoint::StaticClass(), move_points);

	for (auto actor : move_points)
	{
		auto move_point = Cast< AInteractionPawnMovePoint>(actor);
		_MovePoints.Add(move_point->GetPointName(), move_point);
	}
}

void AInteractionPawn::DriveMoveToPoint(float _delta_time)
{
	if (IsInvalid(_TargetMovePoint))
		return;

	_MoveElapsedTime += _delta_time;

	const FVector target_location = _TargetMovePoint->GetActorLocation();
	const FRotator target_rotation = _TargetMovePoint->GetActorRotation();

	const float raw_alpha = FMath::Clamp(_MoveElapsedTime / _MoveDuration, 0.0f, 1.0f);
	const float alpha = FMath::InterpEaseInOut(0.0f, 1.0f, raw_alpha, 2.0f);

	const FVector new_location = FMath::Lerp(_MoveStartLocation, target_location, alpha);

	const FQuat start_quat = _MoveStartRotation.Quaternion();
	const FQuat target_quat = target_rotation.Quaternion();
	const FQuat new_quat = FQuat::Slerp(start_quat, target_quat, alpha);
	const FRotator new_rotation = new_quat.Rotator();

	SetActorLocationAndRotation(new_location, new_rotation);

	if (raw_alpha >= 1.0f)
	{
		SetActorLocationAndRotation(target_location, target_rotation);

		_OnMoveToPointFinishedEvent.ExecuteIfBound(_TargetMovePoint->GetPointName());
		_OnMoveToPointFinishedEvent.Unbind();

		SetUseLookAtCursor(!_TargetMovePoint->GetUseFixedCamera());
		_TargetMovePoint = nullptr;
	}
}

void AInteractionPawn::SetTargetMovePoint(const FName _point_name, bool _is_teleport, const FD_OnMoveToPointFinished& _on_move_finished)
{
	const auto point_ptr = _MovePoints.Find(_point_name);
	if (IsInvalid(point_ptr))
	{
		TRACE_WARNING(TEXT("Has no point! : %s"), *_point_name.ToString());
		return;
	}

	const auto point = *point_ptr;
	if (IsInvalid(point))
	{
		TRACE_WARNING(TEXT("MovePoint invalid! : %s"), *_point_name.ToString());
		return;
	}

	if (_is_teleport)
	{
		SetActorLocationAndRotation(point->GetActorLocation(), point->GetActorRotation());

		_on_move_finished.ExecuteIfBound(_point_name);
		SetUseLookAtCursor(!point->GetUseFixedCamera());
		InteractorComponent->SetDetectMode(point->GetInteractionEnabled() ? _DefaultDetectMode : EInteractionDetectMode::NA);
		return;
	}

	_TargetMovePoint = point;

	_MoveStartLocation = GetActorLocation();
	_MoveStartRotation = GetActorRotation();
	_MoveElapsedTime = 0.0f;

	const float distance = FVector::Distance(_MoveStartLocation, point->GetActorLocation());
	const float location_duration = distance / _MoveSpeed;

	const float angle = _MoveStartRotation.Quaternion().AngularDistance(point->GetActorRotation().Quaternion());
	const float angle_degree = FMath::RadiansToDegrees(angle);
	const float rotation_duration = angle_degree / _RotateSpeed;

	_MoveDuration = FMath::Max(location_duration, rotation_duration);
	_MoveDuration = FMath::Max(_MoveDuration, KINDA_SMALL_NUMBER);

	_OnMoveToPointFinishedEvent = _on_move_finished;

	InteractorComponent->SetDetectMode(_TargetMovePoint->GetInteractionEnabled() ? _DefaultDetectMode : EInteractionDetectMode::NA);
		
	SetUseLookAtCursor(false);
}
