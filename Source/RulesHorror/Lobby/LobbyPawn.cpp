// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPawn.h"
#include "RulesHorrorUtils.h"
#include "Camera/CameraComponent.h"
#include "Lobby/LobbyPlayerController.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InteractionComponent.h"

ALobbyPawn::ALobbyPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));
	InteractionComponent->SetupAttachment(Root);

	AutoPossessPlayer = EAutoReceiveInput::Player0;
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

void ALobbyPawn::InteractInput(const FInputActionValue& _value)
{
	if (_CanInteract == false)
		return;

	InteractionComponent->TryInteract();
}

void ALobbyPawn::Tick(float _delta_time)
{
	Super::Tick(_delta_time);

	if (_CanLook)
	{
		UpdateLookTargetFromMouse();

		_CurrentYaw = FMath::FInterpTo(_CurrentYaw, _TargetYaw, _delta_time, _FollowSpeed);
		_CurrentPitch = FMath::FInterpTo(_CurrentPitch, _TargetPitch, _delta_time, _FollowSpeed);

		SetActorRotation(FRotator(_BasePitch + _CurrentPitch, _BaseYaw + _CurrentYaw, 0.0f));
	}
}

void ALobbyPawn::UpdateLookTargetFromMouse()
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

void ALobbyPawn::SetLookEnabled(bool _is_enabled)
{
	_CanLook = _is_enabled;
}

void ALobbyPawn::SetInteractEnabled(bool _is_enabled)
{
	_CanInteract = _is_enabled;
}
