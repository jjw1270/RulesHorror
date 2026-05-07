#include "InteractorComponent.h"
#include "InteractionSystem.h"
#include "CommonUtils.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InteractableInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UI_InteractionIndicatorPanel.h"
#include "InteractionSystemDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractorComponent)

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	InitSphereRadius(_DetectableRange);

	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);

	ApplyCollisionChannelSettings();

	SetGenerateOverlapEvents(true);

	OnComponentBeginOverlap.AddDynamic(this, &UInteractorComponent::OnBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UInteractorComponent::OnEndOverlap);
}

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	SetDetectableRange(_DetectableRange);
	SetTargetableRange(_TargetableRange);
	_MinViewDotThreshold = FMath::Cos(FMath::DegreesToRadians(_MaxViewHalfAngleDegrees));

	if (_OverlapObjectTypes.IsEmpty())
	{
		TRACE_ERROR(TEXT("_OverlapObjectTypes가 비었습니다."));
	}

	ApplyCollisionChannelSettings();

	_OverlappedActorInfos.Empty();
	_TargetedActor = nullptr;

	InitIndicatorPanel();

	InitOverlayMaterial();

	TSet<AActor*> overlapping_actors;
	GetOverlappingActors(overlapping_actors);

	for (const auto actor : overlapping_actors)
	{
		if (IsInvalid(actor))
			continue;

		if (actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()) == false)
			continue;

		auto& actor_info = _OverlappedActorInfos.FindOrAdd(actor);
		actor_info.OverlapCount = 1;
		
		if(IsValid(_IndicatorPanel))
		{
			_IndicatorPanel->AddInteractionActor(actor, EInteractionState::None);
		}
	}

	if (_OverlappedActorInfos.IsEmpty() == false && _DetectMode != EInteractionDetectMode::NA)
	{
		SetComponentTickEnabled(true);
		UpdateInteraction();
	}
}

void UInteractorComponent::TickComponent(float _delta_time, ELevelTick _tick_type, FActorComponentTickFunction* _this_tick_function)
{
	Super::TickComponent(_delta_time, _tick_type, _this_tick_function);

	UpdateInteraction();
}

void UInteractorComponent::EndPlay(const EEndPlayReason::Type _end_play_reason)
{
	SetComponentTickEnabled(false);

	for (const auto& pair : _OverlappedActorInfos)
	{
		ClearOverlayMaterial(pair.Key);
	}
	_OriginalForceDisableNaniteMap.Empty();

	_TargetedActor = nullptr;
	_OverlappedActorInfos.Empty();

	if (IsValid(_IndicatorPanel))
	{
		_IndicatorPanel->Close(true);
		_IndicatorPanel = nullptr;
	}

	OnComponentBeginOverlap.RemoveDynamic(this, &UInteractorComponent::OnBeginOverlap);
	OnComponentEndOverlap.RemoveDynamic(this, &UInteractorComponent::OnEndOverlap);

	Super::EndPlay(_end_play_reason);
}

#if WITH_EDITOR
void UInteractorComponent::PostEditChangeProperty(FPropertyChangedEvent& _property_changed_event)
{
	Super::PostEditChangeProperty(_property_changed_event);

	SetDetectableRange(_DetectableRange);
	SetTargetableRange(_TargetableRange);
	_MinViewDotThreshold = FMath::Cos(FMath::DegreesToRadians(_MaxViewHalfAngleDegrees));
	ApplyCollisionChannelSettings();

	SetHiddenInGame(!_ShowDebug);
}
#endif

void UInteractorComponent::OnBeginOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index, bool _is_from_sweep, const FHitResult& _sweep_result)
{
	if (IsInvalid(_other_actor))
		return;

	if (_other_actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()) == false)
		return;

	FInteractionActorInfo& actor_info = _OverlappedActorInfos.FindOrAdd(_other_actor);
	const bool was_present = actor_info.OverlapCount > 0;
	actor_info.OverlapCount++;

	if (IsValid(_IndicatorPanel) && was_present == false)
	{
		_IndicatorPanel->AddInteractionActor(_other_actor, EInteractionState::None);
	}

	if (_DetectMode != EInteractionDetectMode::NA)
	{
		SetComponentTickEnabled(true);
		UpdateInteraction();
	}
}

void UInteractorComponent::OnEndOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index)
{
	if (IsInvalid(_other_actor))
		return;

	auto overlapped_actor_info_ptr = _OverlappedActorInfos.Find(_other_actor);
	if (IsInvalid(overlapped_actor_info_ptr))
		return;

	overlapped_actor_info_ptr->OverlapCount = FMath::Max(0, overlapped_actor_info_ptr->OverlapCount - 1);
	if (overlapped_actor_info_ptr->OverlapCount > 0)
	{
		UpdateInteraction();
		return;
	}

	if (_TargetedActor == _other_actor)
	{
		_TargetedActor = nullptr;
	}

	auto actor_state = (*overlapped_actor_info_ptr).State;

	if (actor_state.IsSet() && actor_state != EInteractionState::None)
	{
		IInteractableInterface::Execute_SetInteractionState(_other_actor, EInteractionState::None);
	}

	ClearOverlayMaterial(_other_actor);

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

void UInteractorComponent::SetDetectMode(EInteractionDetectMode _detect_mode)
{
	if (_DetectMode == _detect_mode)
		return;

	_DetectMode = _detect_mode;

	if (_DetectMode == EInteractionDetectMode::NA)
	{
		ClearInteractionState();
		SetComponentTickEnabled(false);

		if (IsValid(_IndicatorPanel))
		{
			_IndicatorPanel->Hide(EWidgetHideType::Collapsed, true);
		}
	}
	else
	{
		if (IsValid(_IndicatorPanel))
		{
			_IndicatorPanel->Show(EWidgetShowType::SelfHitTestInvisible, true);
		}

		SetComponentTickEnabled(!_OverlappedActorInfos.IsEmpty());
	}
}

void UInteractorComponent::SetDetectableRange(float _range)
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

	if (IsValid(_IndicatorPanel))
	{
		_IndicatorPanel->SetPerspectiveDistance(_TargetableRange, _DetectableRange);
	}
}

void UInteractorComponent::SetTargetableRange(float _range)
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

	if (IsValid(_IndicatorPanel))
	{
		_IndicatorPanel->SetPerspectiveDistance(_TargetableRange, _DetectableRange);
	}
}

void UInteractorComponent::TryInteract()
{
	if (IsInvalid(_TargetedActor))
		return;

	if (IInteractableInterface::Execute_CanInteract(_TargetedActor) == false)
		return;

	IInteractableInterface::Execute_Interact(_TargetedActor, GetOwner());
	UpdateInteraction();
}

void UInteractorComponent::ClearInteractionState()
{
	_TargetedActor = nullptr;

	for (auto& pair : _OverlappedActorInfos)
	{
		AActor* actor = pair.Key;
		FInteractionActorInfo& info = pair.Value;

		info.State = EInteractionState::None;
		info.IsDetectedAndVisible = false;

		if (IsValid(actor))
		{
			if (IsValid(_IndicatorPanel))
			{
				_IndicatorPanel->SetInteractionActorState(actor, EInteractionState::None);
			}

			ClearOverlayMaterial(actor);
			IInteractableInterface::Execute_SetInteractionState(actor, EInteractionState::None);
		}
	}

	if (IsValid(_IndicatorPanel))
	{
		_IndicatorPanel->SetTargetedActor(_TargetedActor);
	}
}

void UInteractorComponent::UpdateInteraction()
{
	if (_DetectMode == EInteractionDetectMode::NA)
		return;

	auto interactor = GetOwner();
	if (IsInvalid(interactor))
		return;

	const FVector location = GetComponentLocation();

	FVector view_location, view_forward;
	GetViewVectorInfo(view_location, view_forward);

	switch (_DetectMode)
	{
	case EInteractionDetectMode::CameraCenter:
		UpdateDetectedAndVisibleActors_CameraCenter(location, view_location, view_forward);
		_TargetedActor = SelectTargetedActor_CameraCenter(location, view_location, view_forward);
		break;

	case EInteractionDetectMode::Cursor:
		UpdateDetectedAndVisibleActors_Cursor(location, view_location);
		_TargetedActor = SelectTargetedActor_Cursor(location);
		break;

	default:
		break;
	}

	UpdateInteractionStates();

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
			ClearOverlayMaterial(_TargetedActor);
			IInteractableInterface::Execute_SetInteractionState(_TargetedActor, EInteractionState::None);
		}

		_TargetedActor = nullptr;

		SetComponentTickEnabled(false);
	}

	if (IsValid(_IndicatorPanel))
	{
		_IndicatorPanel->SetTargetedActor(_TargetedActor);
	}
}

void UInteractorComponent::UpdateInteractionStates()
{
	for (auto& actor_info_pair : _OverlappedActorInfos)
	{
		auto actor = actor_info_pair.Key;
		auto& info = actor_info_pair.Value;

		EInteractionState new_state = EInteractionState::None;

		if (actor == _TargetedActor)
		{
			new_state = EInteractionState::Targeted;
		}
		else if (info.IsDetectedAndVisible)
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

			ApplyOverlayMaterial(actor, info.State == EInteractionState::Targeted ? _OverlayMaterial : nullptr);

			IInteractableInterface::Execute_SetInteractionState(actor, new_state);
		}
	}
}

void UInteractorComponent::UpdateDetectedAndVisibleActors_CameraCenter(const FVector& _location, const FVector& _view_location, const FVector& _view_forward)
{
	for (auto it = _OverlappedActorInfos.CreateIterator(); it; ++it)
	{
		AActor* actor = it.Key();
		if (IsInvalid(actor))
		{
			it.RemoveCurrent();
			continue;
		}

		auto& info = it.Value();
		info.IsDetectedAndVisible = false;

		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);
		const float distance_squared = FVector::DistSquared(_location, interaction_location);

		if (distance_squared > _DetectableRangeSquared)
			continue;

		if (IInteractableInterface::Execute_CanBeDetected(actor) == false)
			continue;

		if (IsActorVisible(actor, _view_location) == false)
			continue;

		info.IsDetectedAndVisible = true;
	}
}

AActor* UInteractorComponent::SelectTargetedActor_CameraCenter(const FVector& _location, const FVector& _view_location, const FVector& _view_forward) const
{
	AActor* new_targeted_actor = nullptr;
	float best_dot = -1.0f;

	for (const auto& actor_info_pair : _OverlappedActorInfos)
	{
		if (actor_info_pair.Value.IsDetectedAndVisible == false)
			continue;

		auto actor = actor_info_pair.Key;

		if (IsActorInTargetableRange(actor, _location) == false)
			continue;

		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);
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

void UInteractorComponent::UpdateDetectedAndVisibleActors_Cursor(const FVector& _location, const FVector& _view_location)
{
	FVector2D mouse_pos = FVector2D::ZeroVector;
	const bool has_mouse_pos = GetMouseScreenPosition(mouse_pos);

	for (auto it = _OverlappedActorInfos.CreateIterator(); it; ++it)
	{
		AActor* actor = it.Key();
		if (IsInvalid(actor))
		{
			it.RemoveCurrent();
			continue;
		}

		auto& info = it.Value();
		info.IsDetectedAndVisible = false;

		if (has_mouse_pos == false)
			continue;

		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);
		const float distance_squared = FVector::DistSquared(_location, interaction_location);

		if (distance_squared > _DetectableRangeSquared)
			continue;

		if (IInteractableInterface::Execute_CanBeDetected(actor) == false)
			continue;

		if (IsActorVisible(actor, _view_location) == false)
			continue;

		// 커서 반경 안에 있는지
		FVector2D screen_pos;
		if (ProjectInteractionLocationToScreen(actor, screen_pos))
		{
			const float screen_distance_squared = FVector2D::DistSquared(screen_pos, mouse_pos);
			if (screen_distance_squared <= _CursorDetectRadius * _CursorDetectRadius)
			{
				info.IsDetectedAndVisible = true;
			}
		}
	}
}

AActor* UInteractorComponent::SelectTargetedActor_Cursor(const FVector& _location) const
{
	const auto pc = GetOwnerPlayerController();
	if (IsInvalid(pc))
		return nullptr;

	FHitResult hit;
	if (pc->GetHitResultUnderCursorForObjects(_OverlapObjectTypes, true, hit) == false)
		return nullptr;

	AActor* hit_actor = hit.GetActor();
	if (IsInvalid(hit_actor))
		return nullptr;

	auto info_ptr = _OverlappedActorInfos.Find(hit_actor);
	if (IsValid(info_ptr) && (*info_ptr).IsDetectedAndVisible)
	{
		if (IsActorInTargetableRange(hit_actor, _location))
		{
			return hit_actor;
		}
	}

	return nullptr;
}

bool UInteractorComponent::GetMouseScreenPosition(FVector2D& _out_mouse_pos) const
{
	const auto pc = GetOwnerPlayerController();
	if (IsInvalid(pc))
		return false;

	float mouse_x, mouse_y;
	if (pc->GetMousePosition(mouse_x, mouse_y) == false)
		return false;

	_out_mouse_pos = FVector2D(mouse_x, mouse_y);
	return true;
}

bool UInteractorComponent::ProjectInteractionLocationToScreen(AActor* _actor, FVector2D& _out_screen_pos) const
{
	if (IsInvalid(_actor))
		return false;

	const auto pc = GetOwnerPlayerController();
	if (IsInvalid(pc))
		return false;

	const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(_actor);
	return pc->ProjectWorldLocationToScreen(interaction_location, _out_screen_pos, true);
}

bool UInteractorComponent::IsActorVisible(AActor* _actor, const FVector& _view_location) const
{
	if (IsInvalid(_actor))
		return false;

	FCollisionQueryParams collision_query_params;
	collision_query_params.AddIgnoredActor(GetOwner());
	collision_query_params.bTraceComplex = true;

	const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(_actor);

	FHitResult hit;
	const bool is_hit = GetWorld()->LineTraceSingleByChannel(hit, _view_location, interaction_location, ECC_Visibility, collision_query_params);

	if (is_hit == false)
		return true;

	if (IInteractableInterface::Execute_CanBeCollapsedBySelf(_actor))
		return false;

	return hit.GetActor() == _actor;
}

bool UInteractorComponent::IsActorInTargetableRange(AActor* _actor, const FVector& _location) const
{
	if (IsInvalid(_actor))
		return false;

	const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(_actor);
	const float distance_squared = FVector::DistSquared(_location, interaction_location);

	return distance_squared <= _TargetableRangeSquared;
}

APlayerController* UInteractorComponent::GetOwnerPlayerController() const
{
	const auto owner_pawn = Cast<APawn>(GetOwner());
	if (IsValid(owner_pawn))
	{
		return Cast<APlayerController>(owner_pawn->GetController());
	}

	return nullptr;
}

void UInteractorComponent::GetViewVectorInfo(FVector& _out_location, FVector& _out_forward) const
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

void UInteractorComponent::ApplyCollisionChannelSettings()
{
	SetCollisionResponseToAllChannels(ECR_Ignore);

	for (const auto& object_type : _OverlapObjectTypes)
	{
		const ECollisionChannel channel = UEngineTypes::ConvertToCollisionChannel(object_type);
		SetCollisionResponseToChannel(channel, ECR_Overlap);
	}
}

void UInteractorComponent::InitIndicatorPanel()
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

void UInteractorComponent::InitOverlayMaterial()
{
	auto dev_settings = GetDefault<UInteractionSystemDeveloperSettings>();
	if (IsInvalid(dev_settings))
		return;

	if (dev_settings->_OverlayMaterialClass.IsNull())
	{
		TRACE_ERROR(TEXT("Interaction System Developer Setting에서 _OverlayMaterialClass를 설정해주세요."));
		return;
	}

	_OverlayMaterial = dev_settings->_OverlayMaterialClass.LoadSynchronous();
}

void UInteractorComponent::ApplyOverlayMaterial(AActor* _actor, UMaterialInterface* _material) const
{
	if (IsInvalid(_actor))
		return;

	if (ShouldShowLocalInteractionVisuals() == false && (IsValid(_material) || _OriginalForceDisableNaniteMap.IsEmpty()))
		return;

	for (auto mesh_comp : IInteractableInterface::Execute_GetEffectedMeshComponents(_actor))
	{
		if (IsInvalid(mesh_comp))
			continue;

		// nanite는 overlay material를 지원하지 않음!!
		// overlay material를 쓸때에만 비활성화
		auto static_mesh_comp = Cast<UStaticMeshComponent>(mesh_comp);
		if (IsValid(static_mesh_comp))
		{
			auto static_mesh = static_mesh_comp->GetStaticMesh();
			if (IsValid(static_mesh))
			{
				if (IsValid(_material))
				{
					if (_OriginalForceDisableNaniteMap.Contains(static_mesh_comp) == false)
					{
						_OriginalForceDisableNaniteMap.Add(static_mesh_comp, static_mesh_comp->IsForceDisableNanite());
					}

					static_mesh_comp->SetForceDisableNanite(true);
				}
				else if (const bool* original_force_disable_nanite = _OriginalForceDisableNaniteMap.Find(static_mesh_comp))
				{
					static_mesh_comp->SetForceDisableNanite(*original_force_disable_nanite);
					_OriginalForceDisableNaniteMap.Remove(static_mesh_comp);
				}
			}
		}

		mesh_comp->SetOverlayMaterial(_material);
	}
}

void UInteractorComponent::ClearOverlayMaterial(AActor* _actor) const
{
	ApplyOverlayMaterial(_actor, nullptr);
}

bool UInteractorComponent::ShouldShowLocalInteractionVisuals() const
{
	const auto pc = GetOwnerPlayerController();
	return IsValid(pc) && pc->IsLocalController();
}

#if !UE_BUILD_SHIPPING
void UInteractorComponent::SetShowDebug(bool _show_debug)
{
	if (_ShowDebug == _show_debug)
		return;

	_ShowDebug = _show_debug;
	SetHiddenInGame(!_ShowDebug);
}

void UInteractorComponent::DrawDebugInteraction(const FVector& _view_location, const FVector& _view_forward)
{
	if (_DetectMode == EInteractionDetectMode::NA)
		return;

	auto world = GetWorld();
	if (IsInvalid(world))
		return;

	const float life_time = PrimaryComponentTick.TickInterval;

	// 공통: targetable
	if (FMath::IsNearlyEqual(_TargetableRange, _DetectableRange) == false)
	{
		DrawDebugSphere(world, _view_location, _TargetableRange, 8, FColor::Yellow, false, life_time, 0, 1.0f);
	}

	FVector circle_y, circle_z;
	_view_forward.FindBestAxisVectors(circle_y, circle_z);

	switch (_DetectMode)
	{
	case EInteractionDetectMode::CameraCenter:
	{
		const float circle_radius = _TargetableRange * FMath::Tan(FMath::DegreesToRadians(_MaxViewHalfAngleDegrees));
		const FVector circle_center = _view_location + _view_forward * _TargetableRange;

		DrawDebugCircle(world, circle_center, circle_radius, 32, FColor::Green, false, life_time, 1, 2.0f, circle_y, circle_z, false);
	}
	break;

	case EInteractionDetectMode::Cursor:
	{
		const auto pc = GetOwnerPlayerController();
		if (IsValid(pc))
		{
			float mouse_x, mouse_y;
			if (pc->GetMousePosition(mouse_x, mouse_y))
			{
				// 화면상의 원을 월드에 투영할 기준 평면
				// 카메라 앞 _TargetableRange 거리의 평면
				const FVector plane_origin = _view_location + (_view_forward * _TargetableRange);
				const FVector plane_normal = _view_forward;

				const int32 segment_count = 24;
				TArray<FVector> world_points;
				world_points.Reserve(segment_count);

				for (int32 i = 0; i < segment_count; ++i)
				{
					const float angle = (2.0f * PI * i) / segment_count;

					const float screen_x = mouse_x + FMath::Cos(angle) * _CursorDetectRadius;
					const float screen_y = mouse_y + FMath::Sin(angle) * _CursorDetectRadius;

					FVector ray_origin;
					FVector ray_direction;
					if (pc->DeprojectScreenPositionToWorld(screen_x, screen_y, ray_origin, ray_direction) == false)
						continue;

					// Ray와 평면의 교점 계산
					const float denom = FVector::DotProduct(ray_direction, plane_normal);
					if (FMath::IsNearlyZero(denom))
						continue;

					const float t = FVector::DotProduct(plane_origin - ray_origin, plane_normal) / denom;
					if (t <= 0.0f)
						continue;

					const FVector world_point = ray_origin + (ray_direction * t);
					world_points.Add(world_point);
				}

				// 원 그리기
				for (int32 i = 0; i < world_points.Num(); ++i)
				{
					const FVector& start = world_points[i];
					const FVector& end = world_points[(i + 1) % world_points.Num()];

					DrawDebugLine(world, start, end, FColor::Green, false, life_time, 1, 2.0f);
				}
			}
		}
	}
	break;

	default:
		break;
	}
}
#endif
