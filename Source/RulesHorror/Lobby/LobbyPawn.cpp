// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPawn.h"
#include "RulesHorrorUtils.h"
#include "Camera/CameraComponent.h"
#include "Lobby/LobbyPlayerController.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InteractorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/LobbyPawnMovePoint.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "Lobby/Computer.h"

ALobbyPawn::ALobbyPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	InteractorComponent = CreateDefaultSubobject<UInteractorComponent>(TEXT("Interactor"));
	InteractorComponent->SetupAttachment(Root);

	WidgetInteractionComponent = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("WidgetInteraction"));
	WidgetInteractionComponent->SetupAttachment(GetRootComponent());

	WidgetInteractionComponent->TraceChannel = UEngineTypes::ConvertToCollisionChannel(_MonitorScreenWidgetObjectType);
	WidgetInteractionComponent->InteractionSource = EWidgetInteractionSource::Custom;
}

void ALobbyPawn::BeginPlay()
{
	Super::BeginPlay();

	_BaseYaw = GetActorRotation().Yaw;
	_BasePitch = GetActorRotation().Pitch;

	auto pc = Cast<ALobbyPlayerController>(GetController());
	if (IsInvalid(pc))
		return;

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

	InitMovePoints();
}

void ALobbyPawn::SetupPlayerInputComponent(UInputComponent* _input_component)
{
	Super::SetupPlayerInputComponent(_input_component);

	auto enhanced_input_comp = Cast<UEnhancedInputComponent>(_input_component);
	if (IsInvalid(enhanced_input_comp))
	{
		TRACE_ERROR(TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this))
			return;
	}

	if (IsAnyInvalid(_IA_Interact))
	{
		TRACE_ERROR(TEXT("Some Input Action is not set!!"));
		return;
	}

	enhanced_input_comp->BindAction(_IA_Interact, ETriggerEvent::Started, this, &ALobbyPawn::InteractInput);
}

void ALobbyPawn::Tick(float _delta_time)
{
	Super::Tick(_delta_time);

	if (_UseLookAtCursor)
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

	DriveWidgetInteraction();
}

void ALobbyPawn::UpdateLookAtCursor()
{
	auto pc = Cast<ALobbyPlayerController>(GetController());
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

void ALobbyPawn::SetUseLookAtCursor(bool _value)
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

void ALobbyPawn::InteractInput(const FInputActionValue& _value)
{
	if (_CanInteract == false)
		return;

	InteractorComponent->TryInteract();
}

void ALobbyPawn::SetInteractEnabled(bool _is_enabled)
{
	_CanInteract = _is_enabled;
}

void ALobbyPawn::InitMovePoints()
{
	_MovePoints.Empty();

	TArray<AActor*> move_points;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALobbyPawnMovePoint::StaticClass(), move_points);

	for (auto actor : move_points)
	{
		auto move_point = Cast< ALobbyPawnMovePoint>(actor);
		_MovePoints.Add(move_point->GetPointName(), move_point);
	}
}

void ALobbyPawn::DriveMoveToPoint(float _delta_time)
{
	if (IsInvalid(_TargetMovePoint))
		return;

	// location
	const FVector current_location = GetActorLocation();
	const FVector target_location = _TargetMovePoint->GetActorLocation();

	constexpr float location_tolerance = 1.0f;     // cm
	const bool is_location_arrived = FVector::DistSquared(current_location, target_location) <= location_tolerance * location_tolerance;

	// rotation
	const FRotator current_rotation = GetActorRotation();
	const FRotator target_rotation = _TargetMovePoint->GetActorRotation();

	constexpr float rotation_tolerance = 0.5f;     // degree
	const bool is_rotation_arrived = current_rotation.Equals(target_rotation, rotation_tolerance);

	if (is_location_arrived && is_rotation_arrived)
	{
		_OnMoveToPointFinishedEvent.ExecuteIfBound(this);
		_OnMoveToPointFinishedEvent.Unbind();

		SetUseLookAtCursor(!_TargetMovePoint->GetUseFixedCamera());
		_TargetMovePoint = nullptr;
	}
	else
	{
		FVector new_location = is_location_arrived ? current_location : FMath::VInterpTo(current_location, target_location, _delta_time, _MoveSpeed);
		FRotator new_rotation = is_rotation_arrived ? current_rotation : FMath::RInterpTo(current_rotation, target_rotation, _delta_time, _MoveSpeed);

		SetActorLocationAndRotation(new_location, new_rotation);
	}
}

void ALobbyPawn::SetTargetMovePoint(const FName _point_name, const FD_OnMoveToPointFinished& _on_move_finished)
{
	auto point_ptr = _MovePoints.Find(_point_name);
	if (IsInvalid(point_ptr))
	{
		TRACE_WARNING(TEXT("Has no point! : %s"), *_point_name.ToString());
		return;
	}

	if (IsInvalid(*point_ptr))
	{
		TRACE_WARNING(TEXT("MovePoint invalid! : %s"), *_point_name.ToString());
		return;
	}

	_TargetMovePoint = *point_ptr;
	_OnMoveToPointFinishedEvent = _on_move_finished;

	InteractorComponent->SetDetectMode(_TargetMovePoint->GetInteractionEnabled() ? EInteractionDetectMode::Cursor : EInteractionDetectMode::NA);
		
	SetUseLookAtCursor(false);
}

void ALobbyPawn::DriveWidgetInteraction()
{
	if (IsInvalid(_InteractingComputer))
		return;

	auto pc = Cast<ALobbyPlayerController>(GetController());
	if (IsInvalid(pc))
		return;

	FHitResult hit;
	bool is_hit = pc->GetHitResultUnderCursorForObjects({ _MonitorScreenWidgetObjectType }, true, hit);
	BuildCorrectedMonitorWidgetHit(hit);

	WidgetInteractionComponent->SetCustomHitResult(hit);
}

void ALobbyPawn::BuildCorrectedMonitorWidgetHit(FHitResult& _out_hit) const
{
	auto widget_component = Cast<UWidgetComponent>(_out_hit.GetComponent());
	if (IsInvalid(widget_component))
		return;

	const FVector2D draw_size = FVector2D(widget_component->GetDrawSize());
	if (draw_size.X <= 1.0f || draw_size.Y <= 1.0f)
		return;

	const FTransform component_transform = widget_component->GetComponentTransform();
	const FVector local_hit_location = component_transform.InverseTransformPosition(_out_hit.ImpactPoint);

	// WidgetComponent 기준:
	// X = 평면 법선 방향
	// Y = 좌우
	// Z = 상하
	FVector corrected_local_hit_location = local_hit_location;

	const float half_width = draw_size.X * 0.5f;
	const float half_height = draw_size.Y * 0.5f;

	const float normalized_x = local_hit_location.Y / FMath::Max(half_width, 1.0f);
	const float normalized_y = local_hit_location.Z / FMath::Max(half_height, 1.0f);

	const float abs_normalized_x = FMath::Abs(normalized_x);
	const float abs_normalized_y = FMath::Abs(normalized_y);

	// FMargin 기준:
	// Left   = 좌측 보정 강도
	// Top    = 상단 보정 강도
	// Right  = 우측 보정 강도
	// Bottom = 하단 보정 강도
	const float correction_strength_x = (normalized_x > 0.0f) ? _MonitorWidgetHitCorrection.Left : _MonitorWidgetHitCorrection.Right;
	const float correction_strength_y = (normalized_y > 0.0f) ? _MonitorWidgetHitCorrection.Top : _MonitorWidgetHitCorrection.Bottom;

	// 중심 쪽으로 압축
	float correction_scale_x = 1.0f - (correction_strength_x * abs_normalized_x * abs_normalized_x);
	float correction_scale_y = 1.0f - (correction_strength_y * abs_normalized_y * abs_normalized_y);

	// 너무 과하게 줄어드는 것 방지
	correction_scale_x = FMath::Clamp(correction_scale_x, 0.7f, 1.0f);
	correction_scale_y = FMath::Clamp(correction_scale_y, 0.7f, 1.0f);

	corrected_local_hit_location.Y *= correction_scale_x;
	corrected_local_hit_location.Z *= correction_scale_y;

	const FVector corrected_world_hit_location = component_transform.TransformPosition(corrected_local_hit_location);

	_out_hit.Location = corrected_world_hit_location;
	_out_hit.ImpactPoint = corrected_world_hit_location;
	_out_hit.Distance = FVector::Distance(_out_hit.TraceStart, corrected_world_hit_location);
}

void ALobbyPawn::SetInteractingComputer(AComputer* _computer)
{
	_InteractingComputer = _computer;
}

bool ALobbyPawn::IsOnInteracintingComputer() const
{
	return IsValid(_InteractingComputer);
}
