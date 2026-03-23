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
#include "UI_InteractionIndicatorPanel.h"


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

	SetDetectableRange(_DetectableRange);
	SetTargetableRange(_TargetableRange);
	_MinViewDotThreshold = FMath::Cos(FMath::DegreesToRadians(_MaxViewHalfAngleDegrees));

	_OverlappedActorInfos.Empty();
	_TargetedActor = nullptr;

	InitIndicatorPanel();

	TSet<AActor*> overlapping_actors;
	GetOverlappingActors(overlapping_actors);

	for (const auto actor : overlapping_actors)
	{
		if (IsInvalid(actor))
			continue;

		if (actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()) == false)
			continue;

		_OverlappedActorInfos.Add(actor);
		
		if(IsValid(_IndicatorPanel))
		{
			_IndicatorPanel->AddInteractionActor(actor, EInteractionState::None);
		}
	}

	if (_OverlappedActorInfos.IsEmpty() == false)
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

	_OverlappedActorInfos.Add(_other_actor);

	if (IsValid(_IndicatorPanel))
	{
		_IndicatorPanel->AddInteractionActor(_other_actor, EInteractionState::None);
	}

	SetComponentTickEnabled(true);
	UpdateInteraction();
}

void UInteractionComponent::OnEndOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index)
{
	if (IsInvalid(_other_actor))
		return;

	auto overlapped_actor_info_ptr = _OverlappedActorInfos.Find(_other_actor);
	if (IsInvalid(overlapped_actor_info_ptr))
		return;

	if (_TargetedActor == _other_actor)
	{
		_TargetedActor = nullptr;
	}

	auto actor_state = (*overlapped_actor_info_ptr).State;

	if (actor_state.IsSet() && actor_state != EInteractionState::None)
	{
		IInteractableInterface::Execute_SetInteractionState(_other_actor, EInteractionState::None);
	}

	if (IsValid(_IndicatorPanel))
	{
		_IndicatorPanel->RemoveInteractionActor(_other_actor);
	}

	_OverlappedActorInfos.Remove(_other_actor);

	if (_OverlappedActorInfos.IsEmpty())
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

void UInteractionComponent::TryInteract()
{
	if (IsInvalid(_TargetedActor))
		return;

	if (IInteractableInterface::Execute_CanInteract(_TargetedActor) == false)
		return;

	IInteractableInterface::Execute_Interact(_TargetedActor, GetOwner());
}

void UInteractionComponent::UpdateInteraction()
{
	auto interactor = GetOwner();
	if (IsInvalid(interactor))
		return;

	const FVector location = GetComponentLocation();

	FVector view_location, view_forward;
	GetViewVectorInfo(view_location, view_forward);

	FCollisionQueryParams collision_query_params;
	collision_query_params.AddIgnoredActor(interactor);
	collision_query_params.bTraceComplex = true;
	FHitResult hit;

	for (auto it = _OverlappedActorInfos.CreateIterator(); it; ++it)
	{
		AActor* actor = it.Key();
		if (IsInvalid(actor))
		{
			it.RemoveCurrent();
			continue;
		}

		auto& info = it.Value();

		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);
		const float distance_squared = FVector::DistSquared(location, interaction_location);

		info.IsDetectedAndVisible = true;

		if (distance_squared > _DetectableRangeSquared || IInteractableInterface::Execute_CanBeDetected(actor) == false)
		{
			info.IsDetectedAndVisible = false;
		}
		else
		{
			bool is_hit = GetWorld()->LineTraceSingleByChannel(hit, view_location, interaction_location, ECC_Visibility, collision_query_params);

			if (is_hit)
			{
				if (IInteractableInterface::Execute_CanBeCollapsedBySelf(actor) || hit.GetActor() != actor)
				{
					info.IsDetectedAndVisible = false;
				}
			}
		}
	}

	_TargetedActor = SelectTargetedActor(view_location, view_forward);

	for (auto& actor_info_pair : _OverlappedActorInfos)
	{
		auto actor = actor_info_pair.Key;
		auto& info = actor_info_pair.Value;

		EInteractionState new_state = EInteractionState::None;

		if (actor == _TargetedActor)
		{
			new_state = EInteractionState::Targeted;
		}
		else if(info.IsDetectedAndVisible)
		{
			new_state = EInteractionState::Detected;
		}

		if (info.State.IsSet() == false || info.State.GetValue() != new_state)
		{
			info.State = new_state;

			if (IsValid(_IndicatorPanel))
			{
				_IndicatorPanel->SetInteractionActorState(actor, new_state);
			}

			IInteractableInterface::Execute_SetInteractionState(actor, new_state);
		}
	}

#if !UE_BUILD_SHIPPING
	if (_ShowDebug)
	{
		DrawDebugInteraction(view_location, view_forward);
	}
#endif

	if (_OverlappedActorInfos.IsEmpty())
	{
		if (IsValid(_TargetedActor))
		{
			IInteractableInterface::Execute_SetInteractionState(_TargetedActor, EInteractionState::None);
		}

		_TargetedActor = nullptr;

		SetComponentTickEnabled(false);
	}
}

AActor* UInteractionComponent::SelectTargetedActor(const FVector& _view_location, const FVector& _view_forward) const
{
	AActor* new_targeted_actor = nullptr;
	float best_dot = -1.0f;

	const FVector location = GetComponentLocation();

	for (const auto& actor_info_pair : _OverlappedActorInfos)
	{
		if (actor_info_pair.Value.IsDetectedAndVisible == false)
			continue;

		auto actor = actor_info_pair.Key;

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

APlayerController* UInteractionComponent::GetOwnerPlayerController() const
{
	const auto owner_pawn = Cast<APawn>(GetOwner());
	if (IsValid(owner_pawn))
	{
		return Cast<APlayerController>(owner_pawn->GetController());
	}

	return nullptr;
}

void UInteractionComponent::GetViewVectorInfo(FVector& _out_location, FVector& _out_forward) const
{
	const auto pc = GetOwnerPlayerController();
	if (IsValid(pc))
	{
		FRotator view_rotation = FRotator::ZeroRotator;
		pc->GetPlayerViewPoint(_out_location, view_rotation);

		_out_forward = view_rotation.Vector();

		return;
	}

	const auto owner = GetOwner();
	if (IsValid(owner))
	{
		const auto cam = owner->FindComponentByClass<UCameraComponent>();
		if (IsValid(cam))
		{
			_out_location = cam->GetComponentLocation();
			_out_forward = cam->GetForwardVector();

			return;
		}

		_out_location = owner->GetActorLocation();
		_out_forward = owner->GetActorForwardVector();

		return;
	}

	_out_location = FVector::ZeroVector;
	_out_forward = FVector::ZeroVector;
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
	auto world = GetWorld();
	if (IsInvalid(world))
		return;

	// targetable range
	DrawDebugSphere(world, GetComponentLocation(), _TargetableRange, 12, FColor::Yellow, false, PrimaryComponentTick.TickInterval);

	// 시야각
	const float circle_radius = _TargetableRange * FMath::Tan(FMath::DegreesToRadians(_MaxViewHalfAngleDegrees));
	const FVector circle_center = _view_location + _view_forward * _TargetableRange;

	FVector circle_y, circle_z;
	_view_forward.FindBestAxisVectors(circle_y, circle_z);

	DrawDebugCircle(world, circle_center, circle_radius, 32, FColor::Green, false, PrimaryComponentTick.TickInterval, 1, 2.0f, circle_y, circle_z, false);

	// overlapped actors
	for (const auto& overlapped_actor_pair : _OverlappedActorInfos)
	{
		AActor* actor = overlapped_actor_pair.Key;
		if (IsInvalid(actor))
			continue;

		auto state = overlapped_actor_pair.Value.State;
		if (state.IsSet() == false)
			continue;

		FColor color = FColor::White;
		switch (state.GetValue())
		{
		case EInteractionState::Detected:
			color = FColor::Blue;
			break;

		case EInteractionState::Targeted:
			color = FColor::Yellow;
			break;

		default:
			break;
		}

		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);
		DrawDebugSphere(world, interaction_location, 8.0f, 8, color, false, PrimaryComponentTick.TickInterval, 1);
	}
}
#endif

void UInteractionComponent::InitIndicatorPanel()
{
	if (IsInvalid(_IndicatorPanelClass))
		return;

	const auto pc = GetOwnerPlayerController();
	if (IsInvalid(pc) || pc->IsLocalController() == false)
		return;

	_IndicatorPanel = CreateWidget<UUI_InteractionIndicatorPanel>(pc, _IndicatorPanelClass);
	if (IsInvalid(_IndicatorPanel))
		return;

	_IndicatorPanel->SetPerspectiveDistance(_TargetableRange, _DetectableRange);

	_IndicatorPanel->AddToViewport(_IndicatorPanelZOrder);
}