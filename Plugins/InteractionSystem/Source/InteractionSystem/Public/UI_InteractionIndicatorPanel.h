// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "InteractionSystemDefines.h"
#include "UI_InteractionIndicatorPanel.generated.h"

class UUI_InteractionIndicator;

/**
 * 
 */
UCLASS(abstract)
class INTERACTIONSYSTEM_API UUI_InteractionIndicatorPanel : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> CanvasPanel = nullptr;
	
protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUI_InteractionIndicator> _IndicatorClass = nullptr;

	UPROPERTY(EditAnywhere)
	FVector2D _IndicatorSize = FVector2D(16.0f, 16.0f);

	UPROPERTY(EditAnywhere)
	int32 _IndicatorPoolSize = 10;

	UPROPERTY()
	TArray<TObjectPtr<UUI_InteractionIndicator>> _IndicatorPool;

	UPROPERTY()
	TMap<TObjectPtr<AActor>, TObjectPtr<UUI_InteractionIndicator>> _ActivatedIndicatorMap;

	float _PerspectiveMinDistance = 300.0f;
	float _PerspectiveMaxDistance = 1000.0f;

	UPROPERTY(EditAnywhere)
	float _PerspectiveMinScale = 0.6f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& _geo, float _delta) override;
	virtual void NativeDestruct() override;

public:
	void SetPerspectiveDistance(float _min_dist, float _max_dist);

	void AddInteractionActor(AActor* _interaction_actor, EInteractionState _state);
	void RemoveInteractionActor(AActor* _interaction_actor);

	void SetInteractionActorState(AActor* _interaction_actor, EInteractionState _state);

protected:
	UUI_InteractionIndicator* PickIndicatorFromPool();
};
