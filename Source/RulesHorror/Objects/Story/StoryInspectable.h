// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "StoryFlow/RulesHorrorStoryDefines.h"
#include "StoryInspectable.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/*
 * 조사할 수 있는 스토리 오브젝트 (문서, 사진, 쪽지, 문 등).
 * 상호작용하면 _Lines를 대사 상자로 보여 주고, 다 보면 _EventName 스토리 이벤트를 보낸다(Shot_WaitStoryEvent가 받음).
 * 감지 범위는 InteractBox 크기. 모양은 Mesh에 아무 스태틱 메시나 넣으면 된다(충돌 없음).
 */
UCLASS()
class RULESHORROR_API AStoryInspectable : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> InteractBox = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

protected:
	/* 다 본 뒤 보낼 스토리 이벤트 이름 (예: Inspect_Dismissal) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName _EventName;

	/* 상호작용 표시에 나오는 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FText _InteractionName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FStoryDialogueLine> _Lines;

	/* 끄면 감지·조사가 안 된다. Shot_SetInteractable로 켜고 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	bool _IsInspectEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	bool _IsInspected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	EInteractionState _InteractionState = EInteractionState::None;

public:
	AStoryInspectable();

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetInspectEnabled(bool _is_enabled);

	UFUNCTION(BlueprintPure, Category = "Story")
	bool IsInspectEnabled() const { return _IsInspectEnabled; }

	UFUNCTION(BlueprintPure, Category = "Story")
	bool IsInspected() const { return _IsInspected; }

protected:
	virtual void SetInteractionState_Implementation(EInteractionState _state) override;
	virtual bool CanBeDetected_Implementation() const override;
	virtual bool CanInteract_Implementation() const override;
	virtual void Interact_Implementation(AActor* _interactor) override;
	virtual FText GetDisplayName_Implementation() const override;
	virtual FVector GetInteractionLocation_Implementation() const override;
	virtual TSet<UMeshComponent*> GetEffectedMeshComponents_Implementation() const override;

	void OnInspectDialogueFinished();
};
