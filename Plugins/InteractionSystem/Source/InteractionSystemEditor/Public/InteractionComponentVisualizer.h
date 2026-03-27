// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class UInteractionComponent;

class FInteractionComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* _component, const FSceneView* _view, 	FPrimitiveDrawInterface* _pdi	) override;

protected:
	void DrawCommon(const UInteractionComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const;
	void DrawCameraCenterMode(const UInteractionComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const;
	void DrawCursorMode(const UInteractionComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const;

};
