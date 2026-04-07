// Copyright (c) 2026 장윤제. All rights reserved.

#include "InteractorComponentVisualizer.h"
#include "CommonUtils.h"
#include "InteractorComponent.h"
#include "InteractableInterface.h"
#include "SceneManagement.h"

void FInteractorComponentVisualizer::DrawVisualization(const UActorComponent* _component, const FSceneView* _view, FPrimitiveDrawInterface* _pdi)
{
	const auto interaction_comp = Cast<UInteractorComponent>(_component);
	if (IsInvalid(interaction_comp))
		return;

	if (interaction_comp->_ShowDebug == false)
		return;

	if (interaction_comp->_DetectMode == EInteractionDetectMode::NA)
		return;

	DrawCommon(interaction_comp, _view, _pdi);

	switch (interaction_comp->_DetectMode)
	{
	case EInteractionDetectMode::CameraCenter:
		DrawCameraCenterMode(interaction_comp, _view, _pdi);
		break;

	case EInteractionDetectMode::Cursor:
		DrawCursorMode(interaction_comp, _view, _pdi);
		break;

	default:
		break;
	}
}

void FInteractorComponentVisualizer::DrawCommon(const UInteractorComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const
{
	if (IsInvalid(_interaction_comp))
		return;

	const FVector origin = _interaction_comp->GetComponentLocation();

	// Detectable range
	DrawWireSphere(_pdi, origin, FColor(64, 128, 255), _interaction_comp->_DetectableRange, 32, SDPG_World);

	// Targetable range
	DrawWireSphere(_pdi, origin, FColor::Yellow, _interaction_comp->_TargetableRange, 24, SDPG_World);

	// Current view
	FVector view_location, view_forward;
	_interaction_comp->GetViewVectorInfo(view_location, view_forward);

	const FTransform view_transform(view_forward.ToOrientationQuat(), view_location);
	DrawDirectionalArrow(_pdi, view_transform.ToMatrixWithScale(), FColor::Red, 40.0f, 6.0f, SDPG_World, 1.5f);

	// Overlapped actors state
	for (const auto& actor_info_pair : _interaction_comp->_OverlappedActorInfos)
	{
		AActor* actor = actor_info_pair.Key;
		if (IsInvalid(actor))
			continue;

		const FInteractionActorInfo& info = actor_info_pair.Value;
		const FVector interaction_location = IInteractableInterface::Execute_GetInteractionLocation(actor);

		FColor color = FColor::Silver;

		if (actor == _interaction_comp->_TargetedActor)
		{
			color = FColor::Yellow;
		}
		else if (info.IsDetectedAndVisible)
		{
			color = FColor::Blue;
		}

		DrawWireSphere(_pdi, interaction_location, color, 8.0f, 8, SDPG_World);

		// owner -> actor line
		const FVector owner_location = _interaction_comp->GetComponentLocation();
		_pdi->DrawLine(owner_location, interaction_location, color, SDPG_World, 0.5f);
	}
}

void FInteractorComponentVisualizer::DrawCameraCenterMode(const UInteractorComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const
{
	if (IsInvalid(_interaction_comp))
		return;

	FVector view_location, view_forward;
	_interaction_comp->GetViewVectorInfo(view_location, view_forward);

	const FTransform cone_transform(view_forward.ToOrientationQuat(), view_location);

	TArray<FVector> cone_verts;
	const float half_angle_rad = FMath::DegreesToRadians(_interaction_comp->_MaxViewHalfAngleDegrees);
	const float cone_length = _interaction_comp->_TargetableRange / FMath::Cos(half_angle_rad);

	DrawWireCone(_pdi, cone_verts, cone_transform, cone_length, _interaction_comp->_MaxViewHalfAngleDegrees, 16, FColor::Green, SDPG_World, 1.5f, 0.0f, false);

	const float circle_radius = _interaction_comp->_TargetableRange * FMath::Tan(half_angle_rad);
	const FVector circle_center = view_location + view_forward * _interaction_comp->_TargetableRange;

	FVector circle_y, circle_z;
	view_forward.FindBestAxisVectors(circle_y, circle_z);

	DrawCircle(_pdi, circle_center, circle_y, circle_z, FColor::Green, circle_radius, 32, SDPG_World, 1.5f, false);
}

void FInteractorComponentVisualizer::DrawCursorMode(const UInteractorComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const
{
	if (IsInvalid(_interaction_comp))
		return;

	// Cursor 모드는 에디터 Visualizer에서 "실제 커서 위치"를 신뢰하기 어렵기 때문에
	// 카메라 정면 기준으로 가이드 원만 보여준다.
	// 실제 커서 기반 디버그는 런타임 DrawDebugInteraction 쪽에서 보강.
	FVector view_location, view_forward;
	_interaction_comp->GetViewVectorInfo(view_location, view_forward);

	const FVector guide_center = view_location + (view_forward * _interaction_comp->_TargetableRange);

	FVector axis_y, axis_z;
	view_forward.FindBestAxisVectors(axis_y, axis_z);

	const float detect_radius_world = FMath::Max(_interaction_comp->_TargetableRange * 0.08f, 10.0f);
	const float hover_radius_world = FMath::Max(detect_radius_world * 0.25f, 4.0f);

	DrawCircle(_pdi, guide_center, axis_y, axis_z, FColor::Cyan, detect_radius_world, 32, SDPG_World, 1.5f, false);

	DrawCircle(_pdi, guide_center, axis_y, axis_z, FColor::Yellow, hover_radius_world, 24, SDPG_World, 1.5f, false);

	DrawDirectionalArrow(_pdi, FTransform(view_forward.ToOrientationQuat(), view_location).ToMatrixWithScale(), FColor::Cyan, _interaction_comp->_TargetableRange, 4.0f, SDPG_World, 1.0f);
}