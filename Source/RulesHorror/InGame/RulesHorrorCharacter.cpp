// Copyright Epic Games, Inc. All Rights Reserved.

#include "RulesHorrorCharacter.h"
#include "RulesHorrorUtils.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InteractionComponent.h"

ARulesHorrorCharacter::ARulesHorrorCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 96.0f);

	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = _WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// create the spotlight
	SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLight"));
	SpotLight->SetupAttachment(FirstPersonCameraComponent);

	SpotLight->SetRelativeLocationAndRotation(FVector(30.0f, 17.5f, -5.0f), FRotator(-18.6f, -1.3f, 5.26f));
	SpotLight->Intensity = 0.5;
	SpotLight->SetIntensityUnits(ELightUnits::Lumens);
	SpotLight->AttenuationRadius = 1050.0f;
	SpotLight->InnerConeAngle = 18.7f;
	SpotLight->OuterConeAngle = 45.24f;

	// create interaction
	Interaction = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));
	Interaction->SetupAttachment(GetCapsuleComponent());
}

void ARulesHorrorCharacter::SetupPlayerInputComponent(UInputComponent* _input_component)
{	
	// Set up action bindings
	auto enhanced_input_comp = Cast<UEnhancedInputComponent>(_input_component);
	if (IsInvalid(enhanced_input_comp))
	{
		TRACE_ERROR(TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this))
		return;
	}

	if (IsAnyInvalid(_IA_Move, _IA_Look, _IA_Interact))
	{
		TRACE_ERROR(TEXT("Some Input Action is not set!!"));
		return;
	}

	// Moving
	enhanced_input_comp->BindAction(_IA_Move, ETriggerEvent::Triggered, this, &ARulesHorrorCharacter::MoveInput);

	// Looking/Aiming
	enhanced_input_comp->BindAction(_IA_Look, ETriggerEvent::Triggered, this, &ARulesHorrorCharacter::LookInput);

	// Interact
	enhanced_input_comp->BindAction(_IA_Interact, ETriggerEvent::Started, this, &ARulesHorrorCharacter::InteractInput);
	enhanced_input_comp->BindAction(_IA_Interact, ETriggerEvent::Completed, this, &ARulesHorrorCharacter::InteractInput);
}

void ARulesHorrorCharacter::MoveInput(const FInputActionValue& _value)
{
	// get the Vector2D move axis
	FVector2D movement_vector = _value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(movement_vector.X, movement_vector.Y);
}

void ARulesHorrorCharacter::LookInput(const FInputActionValue& _value)
{
	// get the Vector2D look axis
	FVector2D look_axis_vector = _value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(look_axis_vector.X, look_axis_vector.Y);
}

void ARulesHorrorCharacter::InteractInput(const FInputActionValue& _value)
{
	if (IsInvalid(Interaction))
		return;
	
	bool is_started = _value.Get<bool>();

	if (is_started)
	{
		Interaction->StartInteract();
	}
	else
	{
		Interaction->EndInteract();
	}
}

void ARulesHorrorCharacter::DoMove(float _right, float _forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), _right);
		AddMovementInput(GetActorForwardVector(), _forward);
	}
}

void ARulesHorrorCharacter::DoAim(float _yaw, float _pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(_yaw);
		AddControllerPitchInput(_pitch);
	}
}
