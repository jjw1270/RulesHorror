// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class UInteractorComponent;

class FInteractorComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* _component, const FSceneView* _view, 	FPrimitiveDrawInterface* _pdi	) override;

protected:
	void DrawCommon(const UInteractorComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const;
	void DrawCameraCenterMode(const UInteractorComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const;
	void DrawCursorMode(const UInteractorComponent* _interaction_comp, const FSceneView* _view, FPrimitiveDrawInterface* _pdi) const;

};
