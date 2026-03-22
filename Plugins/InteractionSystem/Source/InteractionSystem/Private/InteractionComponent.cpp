#include "InteractionComponent.h"
#include "InteractionSystem.h"
#include "CommonUtils.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InteractableInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionComponent)

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	InitSphereRadius(_DetectableRange);

	OnComponentBeginOverlap.AddDynamic(this, &UInteractionComponent::OnBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UInteractionComponent::OnEndOverlap);
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	_TargetedActor = nullptr;
	_InteractingActor = nullptr;

	SetDetectableRange(_DetectableRange);
	SetTargetableRange(_TargetableRange);
	_MinViewDotThreshold = FMath::Cos(FMath::DegreesToRadians(_MaxViewHalfAngleDegrees));

	TSet<AActor*> overlapping_actors;
	GetOverlappingActors(overlapping_actors);

	for (AActor* actor : overlapping_actors)
	{
		if (IsInvalid(actor))
			continue;

		if (actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()) == false)
			continue;

		_OverlappedActors.FindOrAdd(actor) = EInteractionState::None;
	}

	if (_OverlappedActors.IsEmpty() == false)
	{
		SetComponentTickEnabled(true);
		UpdateInteraction();
	}
}

void UInteractionComponent::TickComponent(float _delta_time, ELevelTick _tick_type, FActorComponentTickFunction* _this_tick_function)
{
	Super::TickComponent(_delta_time, _tick_type, _this_tick_function);

	UpdateInteraction();
}

#if WITH_EDITOR
void UInteractionComponent::PostEditChangeProperty(FPropertyChangedEvent& _property_changed_event)
{
	Super::PostEditChangeProperty(_property_changed_event);

	SetDetectableRange(_DetectableRange);
	SetTargetableRange(_TargetableRange);
	_MinViewDotThreshold = FMath::Cos(FMath::DegreesToRadians(_MaxViewHalfAngleDegrees));

	SetHiddenInGame(!_ShowDebug);
}
#endif

void UInteractionComponent::OnBeginOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index, bool _is_from_sweep, const FHitResult& _sweep_result)
{
	if (IsInvalid(_other_actor))
		return;

	if (_other_actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()) == false)
		return;

	_OverlappedActors.FindOrAdd(_other_actor) = EInteractionState::None;

	SetComponentTickEnabled(true);

	UpdateInteraction();
}

void UInteractionComponent::OnEndOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index)
{
	if (IsInvalid(_other_actor))
		return;

	if (_InteractingActor == _other_actor)
	{
		StopCurrentInteraction();
	}

	if (_TargetedActor == _other_actor)
	{
		_TargetedActor = nullptr;
	}

	if (EInteractionState* state_ptr = _OverlappedActors.Find(_other_actor))
	{
		if (*state_ptr != EInteractionState::None)
		{
			IInteractableInterface::Execute_SetInteractionState(_other_actor, EInteractionState::None);
		}
	}

	_OverlappedActors.Remove(_other_actor);

	if (_OverlappedActors.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}

	UpdateInteraction();
}

void UInteractionComponent::SetDetectableRange(float _range)
{
	if (_range < _TargetableRange)
	{
		EDITOR_MESSAGE_ERROR(InteractionSystemLog, TEXT("감지거리가 상호작용 가능 거리보다 작을 수 없습니다!"));
		TRACE_WARNING(TEXT("감지거리가 상호작용 가능 거리보다 작을 수 없습니다!"));

		_DetectableRange = _TargetableRange;
	}
	else
	{
		_DetectableRange = _range;
	}

	_DetectableRangeSquared = _DetectableRange * _DetectableRange;

	SetSphereRadius(_DetectableRange);
}

void UInteractionComponent::SetTargetableRange(float _range)
{
	if (_range > _DetectableRange)
	{
		EDITOR_MESSAGE_ERROR(InteractionSystemLog, TEXT("상호작용 가능 거리가 감지거리보다 클 수 없습니다!"));
		TRACE_WARNING(TEXT("상호작용 가능 거리가 감지거리보다 클 수 없습니다!"));

		_TargetableRange = _DetectableRange;
	}
	else
	{
		_TargetableRange = _range;
	}

	_TargetableRangeSquared = _TargetableRange * _TargetableRange;
}

void UInteractionComponent::StartInteract()
{
	if (IsInvalid(_TargetedActor))
		return;

	if (IInteractableInterface::Execute_CanStartInteract(_TargetedActor) == false)
		return;

	if (IsValid(_InteractingActor))
	{
		if (_InteractingActor == _TargetedActor)
			return;

		StopCurrentInteraction();
	}

	IInteractableInterface::Execute_StartInteract(_TargetedActor, GetOwner());
	_InteractingActor = _TargetedActor;

	UpdateInteraction();
}

void UInteractionComponent::EndInteract()
{
	if (IsInvalid(_InteractingActor))
		return;

	StopCurrentInteraction();

	UpdateInteraction();
}

void UInteractionComponent::UpdateInteraction()
{
	AActor* interactor = GetOwner();
	if (IsValid(interactor) == false)
		return;

	const FVector view_location = GetViewLocation();
	const FVector view_forward = GetViewForwardVector();

	AActor* previous_targeted_actor = _TargetedActor;
	AActor* new_targeted_actor = SelectTargetedActor(view_location, view_forward);

	if (_InteractingActor != nullptr && _InteractingActor != new_targeted_actor)
	{
		StopCurrentInteraction();
	}

	_TargetedActor = new_targeted_actor;

	for (auto it = _OverlappedActors.CreateIterator(); it; ++it)
	{
		AActor* actor = it.Key();

		if (IsInvalid(actor))
		{
			if (_InteractingActor == actor)
			{
				_InteractingActor = nullptr;
			}

			if (_TargetedActor == actor)
			{
				_TargetedActor = nullptr;
			}

			it.RemoveCurrent();
			continue;
		}

		EInteractionState& old_state = it.Value();
		const EInteractionState new_state = EvaluateInteractionState(actor);

		if (old_state != new_state)
		{
			old_state = new_state;
			IInteractableInterface::Execute_SetInteractionState(actor, new_state);
		}
	}

	if (_OverlappedActors.IsEmpty())
	{
		_TargetedActor = nullptr;

		if (IsValid(_InteractingActor))
		{
			StopCurrentInteraction();
		}

		SetComponentTickEnabled(false);
	}

#if !UE_BUILD_SHIPPING
	if (_ShowDebug)
	{
		DrawDebugInteraction(view_location, view_forward);
	}
#endif
}

AActor* UInteractionComponent::SelectTargetedActor(const FVector& _view_location, const FVector& _view_forward) const
{
	AActor* new_targeted_actor = nullptr;
	float best_dot = -1.0f;

	const FVector location = GetComponentLocation();

	for (const auto& overlapped_actor_pair : _OverlappedActors)
	{
		AActor* actor = overlapped_actor_pair.Key;
		if (IsInvalid(actor))
			continue;

		if (IInteractableInterface::Execute_CanBeDetected(actor) == false)
			continue;

		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);
		const float distance_squared = FVector::DistSquared(location, interaction_location);

		if (distance_squared > _TargetableRangeSquared)
			continue;

		const FVector to_target = (interaction_location - _view_location).GetSafeNormal();
		const float dot = FVector::DotProduct(_view_forward, to_target);

		if (dot < _MinViewDotThreshold)
			continue;

		if (dot > best_dot)
		{
			best_dot = dot;
			new_targeted_actor = actor;
		}
	}

	return new_targeted_actor;
}

EInteractionState UInteractionComponent::EvaluateInteractionState(AActor* _actor) const
{
	if (IsInvalid(_actor))
		return EInteractionState::None;

	if (_actor == _InteractingActor)
		return EInteractionState::Interacting;

	if (_actor == _TargetedActor)
		return EInteractionState::Targeted;

	const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(_actor);
	const float distance_squared = FVector::DistSquared(GetComponentLocation(), interaction_location);

	if (distance_squared <= _DetectableRangeSquared && IInteractableInterface::Execute_CanBeDetected(_actor))
	{
		return EInteractionState::Detected;
	}

	return EInteractionState::None;
}

void UInteractionComponent::StopCurrentInteraction()
{
	if (IsInvalid(_InteractingActor))
	{
		_InteractingActor = nullptr;
		return;
	}

	IInteractableInterface::Execute_EndInteract(_InteractingActor, GetOwner());
	_InteractingActor = nullptr;
}

APlayerController* UInteractionComponent::GetOwnerPlayerController() const
{
	const APawn* owner_pawn = Cast<APawn>(GetOwner());
	if (IsValid(owner_pawn))
	{
		return Cast<APlayerController>(owner_pawn->GetController());
	}

	return nullptr;
}

FVector UInteractionComponent::GetViewLocation() const
{
	const APlayerController* pc = GetOwnerPlayerController();
	if (IsValid(pc))
	{
		FVector view_location = FVector::ZeroVector;
		FRotator view_rotation = FRotator::ZeroRotator;
		pc->GetPlayerViewPoint(view_location, view_rotation);

		return view_location;
	}

	const AActor* owner = GetOwner();
	if (IsValid(owner))
	{
		const UCameraComponent* cam = owner->FindComponentByClass<UCameraComponent>();
		if (IsValid(cam))
		{
			return cam->GetComponentLocation();
		}

		return owner->GetActorLocation();
	}

	return FVector::ZeroVector;
}

FVector UInteractionComponent::GetViewForwardVector() const
{
	const APlayerController* pc = GetOwnerPlayerController();
	if (IsValid(pc))
	{
		FVector view_location = FVector::ZeroVector;
		FRotator view_rotation = FRotator::ZeroRotator;
		pc->GetPlayerViewPoint(view_location, view_rotation);

		return view_rotation.Vector();
	}

	const AActor* owner = GetOwner();
	if (IsValid(owner))
	{
		const UCameraComponent* cam = owner->FindComponentByClass<UCameraComponent>();
		if (IsValid(cam))
		{
			return cam->GetForwardVector();
		}

		return owner->GetActorForwardVector();
	}

	return FVector::ForwardVector;
}

#if !UE_BUILD_SHIPPING
void UInteractionComponent::SetShowDebug(bool _show_debug)
{
	if (_ShowDebug == _show_debug)
		return;

	_ShowDebug = _show_debug;
	SetHiddenInGame(!_ShowDebug);
}

void UInteractionComponent::DrawDebugInteraction(const FVector& _view_location, const FVector& _view_forward)
{
	UWorld* world = GetWorld();
	if (IsInvalid(world))
		return;

	DrawDebugSphere(world, GetComponentLocation(), _TargetableRange, 12, FColor::Yellow, false, PrimaryComponentTick.TickInterval);

	const float half_angle_rad = FMath::DegreesToRadians(_MaxViewHalfAngleDegrees);
	const float circle_radius = _TargetableRange * FMath::Tan(half_angle_rad);
	const FVector circle_center = _view_location + _view_forward * _TargetableRange;

	FVector circle_y;
	FVector circle_z;
	_view_forward.FindBestAxisVectors(circle_y, circle_z);

	DrawDebugCircle(world, circle_center, circle_radius, 32, FColor::Green, false, PrimaryComponentTick.TickInterval, 1, 2.0f, circle_y, circle_z, false);

	for (const auto& overlapped_actor_pair : _OverlappedActors)
	{
		AActor* actor = overlapped_actor_pair.Key;
		if (IsInvalid(actor))
			continue;

		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);

		FColor color = FColor::Blue;
		switch (overlapped_actor_pair.Value)
		{
		case EInteractionState::Detected:
			color = FColor::Blue;
			break;
		case EInteractionState::Targeted:
			color = FColor::Yellow;
			break;
		case EInteractionState::Interacting:
			color = FColor::Red;
			break;
		default:
			color = FColor::White;
			break;
		}

		DrawDebugSphere(world, interaction_location, 8.0f, 8, color, false, PrimaryComponentTick.TickInterval, 1);
	}
}
#endif
