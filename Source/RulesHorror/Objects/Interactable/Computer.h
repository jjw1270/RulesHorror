// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "GameFramework/Pawn/OfficePawn.h"
#include "Computer.generated.h"

UCLASS()
class RULESHORROR_API AComputer : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class USceneComponent> Root = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UStaticMeshComponent> MonitorMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UWidgetComponent> ScreenWidgetComponent = nullptr;
	
protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UMaterialInstance> _ScreenMaterialInstance = nullptr;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UMaterialInstanceDynamic> _DynamicScreenMaterialInstance = nullptr;

	const FName TextureParamName = TEXT("Display Input");
	const FName FilterParamName = TEXT("Filter On");

	/* SetPower로 마지막에 정한 전원 상태. 화면 머티리얼은 BeginPlay 다음 틱에 만들어지므로 그때 이 상태를 보고 끌지 정한다. */
	bool _IsPowerOn = false;

public:	
	AComputer();

protected:
	virtual void BeginPlay() override;

protected:
	void UpdateScreenFromWidget();

public:
	UFUNCTION(BlueprintCallable, meta = (Tooltip = "전원 켜기/끄기"))
	void SetPower(bool _on, bool _show_anim = true);

#pragma region Interaction
protected:
	UPROPERTY()
	TObjectPtr<AOfficePawn> _OfficePawn = nullptr;

	UPROPERTY(BlueprintReadOnly)
	EInteractionState _InteractionState = EInteractionState::None;

	/* 끄면 감지·상호작용이 안 된다 (스토리에서 컴퓨터를 쓰면 안 되는 구간). Shot_SetInteractable로 켜고 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool _IsInteractionEnabled = true;

	UPROPERTY(EditAnywhere)
	FName _InteractMovePoint;

	UPROPERTY(EditAnywhere)
	FName _FinishInteractMovePoint;

	FD_OnMoveToPointFinished _OnMoveToPointFinishedEvent;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UUI_OnInteractingComputer> _OnInteractingWidgetClass = nullptr;

	UPROPERTY()
	TObjectPtr<UUI_OnInteractingComputer> _OnInteractingWidget = nullptr;

protected:
	virtual void SetInteractionState_Implementation(EInteractionState _state) override;
	virtual void Interact_Implementation(AActor* _interactor) override;
	virtual bool CanBeDetected_Implementation() const override;
	virtual bool CanInteract_Implementation() const override;

	UFUNCTION()
	void OnOfficePawnMoveToPointFinished(const FName& _point_name);

	void OpenInteractingWidget();
	void CloseInteractingWidget();

public:
	UFUNCTION(BlueprintCallable)
	void FinishInteract();

	UFUNCTION(BlueprintCallable)
	void SetInteractionEnabled(bool _is_enabled) { _IsInteractionEnabled = _is_enabled; }

	UFUNCTION(BlueprintPure)
	bool IsInteractionEnabled() const { return _IsInteractionEnabled; }

#pragma endregion Interaction

};
