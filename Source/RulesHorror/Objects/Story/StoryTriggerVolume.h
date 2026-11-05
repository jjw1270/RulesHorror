// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryTriggerVolume.generated.h"

class UBoxComponent;

/* 플레이어가 들어오면 스토리 이벤트를 보내는 상자 (예: 횡단보도 앞 도달). Shot_WaitStoryEvent가 받는다. */
UCLASS()
class RULESHORROR_API AStoryTriggerVolume : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> TriggerBox = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName _EventName;

	/* 끄면 들어올 때마다 보낸다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	bool _TriggerOnce = true;

	bool _HasTriggered = false;

public:
	AStoryTriggerVolume();

protected:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index, bool _is_from_sweep, const FHitResult& _sweep_result);
};
