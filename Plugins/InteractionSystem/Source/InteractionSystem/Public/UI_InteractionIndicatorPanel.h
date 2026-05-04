// Copyright (c) 2026 장윤제. All rights reserved.

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
	TObjectPtr<class UCanvasPanel> CP_Indicators = nullptr;
	
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

// Targeted Actor
protected:
	UPROPERTY()
	TObjectPtr<AActor> _TargetedActor = nullptr;

public:
	void SetTargetedActor(AActor* _targeted_actor);

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void ShowTargetedPanel(bool _show, const FText& _display_name);
};
