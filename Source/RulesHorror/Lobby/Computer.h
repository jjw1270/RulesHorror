// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "Lobby/LobbyPawn.h"
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
	TObjectPtr<class ALobbyPawn> _InteractorLobbyPawn = nullptr;

	UPROPERTY(BlueprintReadOnly)
	EInteractionState _InteractionState = EInteractionState::None;

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

	UFUNCTION()
	void OnLobbyPawnMoveToPointFinished(const FName& _point_name);

	void OpenInteractingWidget();
	void CloseInteractingWidget();

public:
	UFUNCTION(BlueprintCallable)
	void FinishInteract();

#pragma endregion Interaction

};
