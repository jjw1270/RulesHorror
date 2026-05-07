// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InteractionSystemDefines.h"
#include "InteractorComponent.generated.h"

USTRUCT()
struct FInteractionActorInfo
{
	GENERATED_BODY()

	TOptional<EInteractionState> State;
	bool IsDetectedAndVisible = false;
	int32 OverlapCount = 0;
};

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class INTERACTIONSYSTEM_API UInteractorComponent : public USphereComponent
{
	GENERATED_BODY()

#if WITH_EDITOR
	friend class FInteractorComponentVisualizer;
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> _OverlapObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EInteractionDetectMode _DetectMode = EInteractionDetectMode::CameraCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", Tooltip = "감지 가능한 거리"))
	float _DetectableRange = 600.0f;
	float _DetectableRangeSquared = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", Tooltip = "상호작용 대상으로 선택 가능한 거리"))
	float _TargetableRange = 300.0f;
	float _TargetableRangeSquared = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|CameraCenter", meta = (ClampMin = "0.0", ClampMax = "180.0", Tooltip = "상호작용 가능한 시야 각도"))
	float _MaxViewHalfAngleDegrees = 60.0f;
	float _MinViewDotThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Cursor", meta = (ClampMin = "0.0", Tooltip = "커서 기준 감지 반경(px)"))
	float _CursorDetectRadius = 120.0f;

	UPROPERTY()
	TMap<TObjectPtr<AActor>, FInteractionActorInfo> _OverlappedActorInfos;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> _TargetedActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool _ShowDebug = false;

public:
	UInteractorComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float _delta_time, ELevelTick _tick_type, FActorComponentTickFunction* _this_tick_function) override;
	virtual void EndPlay(const EEndPlayReason::Type _end_play_reason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& _property_changed_event) override;
#endif

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index, bool _is_from_sweep, const FHitResult& _sweep_result);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index);

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetDetectMode(EInteractionDetectMode _detect_mode);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetDetectableRange(float _range);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetTargetableRange(float _range);

	void TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	EInteractionDetectMode GetDetectMode() const { return _DetectMode; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetTargetedActor() const { return _TargetedActor; }

protected:
	void ClearInteractionState();

	void UpdateInteraction();
	void UpdateInteractionStates();

#pragma region CameraCenter
	void UpdateDetectedAndVisibleActors_CameraCenter(const FVector& _location, const FVector& _view_location, const FVector& _view_forward);
	AActor* SelectTargetedActor_CameraCenter(const FVector& _location, const FVector& _view_location, const FVector& _view_forward) const;

#pragma endregion CameraCenter
//////////////////////////////////////////////////////////////////////////////////////////
#pragma region Cursor
	void UpdateDetectedAndVisibleActors_Cursor(const FVector& _location, const FVector& _view_location);
	AActor* SelectTargetedActor_Cursor(const FVector& _location) const;

	bool GetMouseScreenPosition(FVector2D& _out_mouse_pos) const;
	bool ProjectInteractionLocationToScreen(AActor* _actor, FVector2D& _out_screen_pos) const;

#pragma endregion Cursor

	bool IsActorVisible(AActor* _actor, const FVector& _view_location) const;
	bool IsActorInTargetableRange(AActor* _actor, const FVector& _location) const;

	APlayerController* GetOwnerPlayerController() const;
	void GetViewVectorInfo(FVector& _out_location, FVector& _out_forward) const;
	void ApplyCollisionChannelSettings();

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
//////////////////////////////////////////////////////////////////////////////////////////
#pragma region Debug
public:
	UFUNCTION(BlueprintCallable)
	void ToggleDebug()
	{
#if !UE_BUILD_SHIPPING
		SetShowDebug(!_ShowDebug);
#endif
	}
#if !UE_BUILD_SHIPPING
protected:
	void SetShowDebug(bool _show_debug);
	void DrawDebugInteraction(const FVector& _view_location, const FVector& _view_forward);
#endif

#pragma endregion Debug
//////////////////////////////////////////////////////////////////////////////////////////
#pragma region OverlayMaterial
protected:
	UPROPERTY()
	TObjectPtr<class UMaterialInterface> _OverlayMaterial = nullptr;

	mutable TMap<TWeakObjectPtr<class UStaticMeshComponent>, bool> _OriginalForceDisableNaniteMap;

protected:
	void InitOverlayMaterial();
	void ApplyOverlayMaterial(AActor* _actor, class UMaterialInterface* _material) const;
	void ClearOverlayMaterial(AActor* _actor) const;
	bool ShouldShowLocalInteractionVisuals() const;

#pragma endregion OverlayMaterial

};
