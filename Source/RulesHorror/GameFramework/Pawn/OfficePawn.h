// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn/InteractionPawn/InteractionPawn.h"
#include "OfficePawn.generated.h"

class UInputAction;

UCLASS(abstract)
class RULESHORROR_API AOfficePawn : public AInteractionPawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> _IA_LeftMouseClick = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> _IA_MouseWheel = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UWidgetInteractionComponent> WidgetInteractionComponent = nullptr;

	/** 앉아 있는 몸 (본인 화면엔 안 보이고 반사·그림자에만 보임). 기울지 않고 시선 좌우의 일부만 따라 돎 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reflection")
	TObjectPtr<class UStaticMeshComponent> ReflectionBodyMesh = nullptr;

	/** 머리 (시선 방향을 따라 목(_HeadPivot)을 축으로 돎, 반사·그림자에만 보임) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reflection")
	TObjectPtr<class UStaticMeshComponent> ReflectionHeadMesh = nullptr;

	/** 시선 좌우 회전 중 몸이 따라 도는 비율 (0 = 고정, 1 = 머리와 같이) */
	UPROPERTY(EditAnywhere, Category = "Reflection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float _BodyYawFollowRatio = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Reflection", meta = (ClampMin = "0.0"))
	float _BodyTurnSpeed = 6.0f;

	/** 머리가 도는 축 (눈 기준 몸 좌표, cm). 목 가운데 깃 높이라 고개를 돌려도 목이 셔츠 깃 안에 남음 */
	UPROPERTY(EditAnywhere, Category = "Reflection")
	FVector _HeadPivot = FVector(-10.0f, 0.0f, -13.0f);

	float _BodyYaw = 0.0f;

	UPROPERTY()
	TObjectPtr<class AComputer> _InteractingComputer = nullptr;

	UPROPERTY()
	TObjectPtr<class UUI_Monitor> _InteractingMonitorWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Monitor")
	TEnumAsByte<ETraceTypeQuery> _MonitorScreenWidgetTraceType;

	// 보정 강도
	UPROPERTY(EditAnywhere, Category = "Monitor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D _MonitorHitCorrectionStrength = FVector2D(0.1f, 0.1f);

	/* 끄면 모니터 화면 클릭·휠이 화면으로 가지 않는다 (스토리 입력 잠금·대사 중). 눌린 버튼을 떼는 입력은 막지 않는다. */
	bool _IsScreenInputEnabled = true;

public:
	AOfficePawn();

protected:
	virtual void OnConstruction(const FTransform& _transform) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* _input_component) override;
	virtual void Tick(float _delta_time) override;

protected:
	UFUNCTION()
	void Input_LeftMouseButtonStarted();

	UFUNCTION()
	void Input_LeftMouseButtonCompleted();

	UFUNCTION()
	void Input_MouseWheel(const FInputActionValue& _value);

	void DriveWidgetInteraction(float _delta_time);
	void UpdateReflectionBody(float _delta_time, bool _snap);
	void BuildCorrectedMonitorWidgetHit(FHitResult& _out_hit) const;
	void ApplyMonitorTraceChannel();

public:
	UFUNCTION(BlueprintCallable)
	void SetInteractingComputer(AComputer* _computer);

	UFUNCTION(BlueprintPure)
	bool IsOnInteracintingComputer() const;

	UFUNCTION(BlueprintCallable)
	void SetScreenInputEnabled(bool _is_enabled) { _IsScreenInputEnabled = _is_enabled; }

};
