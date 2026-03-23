// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InteractionSystemDefines.h"
#include "InteractionComponent.generated.h"

USTRUCT()
struct FInteractionActorInfo
{
	GENERATED_BODY()

	TOptional<EInteractionState> State;
	bool IsDetectedAndVisible = false;
};

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class INTERACTIONSYSTEM_API UInteractionComponent : public USphereComponent
{
	GENERATED_BODY()

#if WITH_EDITOR
	friend class FInteractionComponentVisualizer;
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", Tooltip = "감지 가능한 거리"))
	float _DetectableRange = 600.0f;
	float _DetectableRangeSquared = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", Tooltip = "상호작용 대상으로 선택 가능한 거리"))
	float _TargetableRange = 300.0f;
	float _TargetableRangeSquared = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", ClampMax = "180.0", Tooltip = "상호작용 가능한 시야 각도"))
	float _MaxViewHalfAngleDegrees = 60.0f;
	float _MinViewDotThreshold = 0.0f;

	UPROPERTY()
	TMap<TObjectPtr<AActor>, FInteractionActorInfo> _OverlappedActorInfos;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> _TargetedActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool _ShowDebug = false;

public:
	UInteractionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float _delta_time, ELevelTick _tick_type, FActorComponentTickFunction* _this_tick_function) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& _property_changed_event) override;
#endif

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index, bool _is_from_sweep, const FHitResult& _sweep_result);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index);

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetDetectableRange(float _range);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetTargetableRange(float _range);

	void TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetTargetedActor() const { return _TargetedActor; }

	UFUNCTION(BlueprintCallable)
	void ToggleDebug()
	{
		SetShowDebug(!_ShowDebug);
	}

protected:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateInteraction();

	AActor* SelectTargetedActor(const FVector& _view_location, const FVector& _view_forward) const;

	APlayerController* GetOwnerPlayerController() const;
	void GetViewVectorInfo(FVector& _out_location, FVector& _out_forward) const;

#if !UE_BUILD_SHIPPING
	void SetShowDebug(bool _show_debug);

	void DrawDebugInteraction(const FVector& _view_location, const FVector& _view_forward);
#endif

#pragma region Indicator
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Indicator")
	TSubclassOf<class UUI_InteractionIndicatorPanel> _IndicatorPanelClass = nullptr;

	UPROPERTY()
	TObjectPtr<UUI_InteractionIndicatorPanel> _IndicatorPanel = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Indicator")
	int32 _IndicatorPanelZOrder = 0;

protected:
	void InitIndicatorPanel();

#pragma endregion Indicator
};
