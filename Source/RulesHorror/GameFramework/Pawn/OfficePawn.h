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

	UPROPERTY()
	TObjectPtr<class AComputer> _InteractingComputer = nullptr;

	UPROPERTY()
	TObjectPtr<class UUI_Monitor> _InteractingMonitorWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Monitor")
	TEnumAsByte<ETraceTypeQuery> _MonitorScreenWidgetTraceType;

	// 보정 강도
	UPROPERTY(EditAnywhere, Category = "Monitor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D _MonitorHitCorrectionStrength = FVector2D(0.1f, 0.1f);

public:
	AOfficePawn();

protected:
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
	void BuildCorrectedMonitorWidgetHit(FHitResult& _out_hit) const;
	void ApplyMonitorTraceChannel();

public:
	UFUNCTION(BlueprintCallable)
	void SetInteractingComputer(AComputer* _computer);

	UFUNCTION(BlueprintPure)
	bool IsOnInteracintingComputer() const;

};
