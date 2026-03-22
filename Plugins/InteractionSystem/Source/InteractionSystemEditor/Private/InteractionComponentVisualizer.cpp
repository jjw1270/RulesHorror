// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractionComponentVisualizer.h"
#include "CommonUtils.h"
#include "InteractionComponent.h"

void FInteractionComponentVisualizer::DrawVisualization(const UActorComponent* _component, const FSceneView* _view, FPrimitiveDrawInterface* _pdi)
{
	const auto interaction_component = Cast<UInteractionComponent>(_component);
	if (IsInvalid(interaction_component))
		return;

	if (interaction_component->_ShowDebug == false)
		return;

	// targetable range
	DrawWireSphere(_pdi, interaction_component->GetComponentLocation(), FColor::Yellow, interaction_component->_TargetableRange, 32, SDPG_World);

	// 시야각
	TArray<FVector> cone_verts;
	const FTransform cone_transform(interaction_component->GetComponentQuat(), interaction_component->GetViewLocation());

	const float half_angle_rad = FMath::DegreesToRadians(interaction_component->_MaxViewHalfAngleDegrees);
	const float cone_length = interaction_component->_TargetableRange / FMath::Cos(half_angle_rad);

	DrawWireCone(_pdi, cone_verts, cone_transform, cone_length, interaction_component->_MaxViewHalfAngleDegrees, 16, FColor::Green, SDPG_World, 1.5f, 0.0f, false);

	DrawDirectionalArrow(_pdi, cone_transform.ToMatrixWithScale(), FColor::Red, interaction_component->_TargetableRange, 4.0f, SDPG_World, 1.5f);
}
