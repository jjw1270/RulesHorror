// Copyright (c) 2026 장윤제. All rights reserved.


#include "Objects/Story/StoryTriggerVolume.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "CommonUtils.h"

AStoryTriggerVolume::AStoryTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->InitBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->ShapeColor = FColor(255, 160, 40);

	// 생성자에서 묶어야 레벨 시작 때 이미 안에 있던 플레이어(처음 겹침)도 놓치지 않는다.
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AStoryTriggerVolume::OnTriggerBeginOverlap);
}

void AStoryTriggerVolume::OnTriggerBeginOverlap(UPrimitiveComponent* _overlapped_component, AActor* _other_actor, UPrimitiveComponent* _other_comp, int32 _other_body_index, bool _is_from_sweep, const FHitResult& _sweep_result)
{
	const APawn* pawn = Cast<APawn>(_other_actor);
	if (IsInvalid(pawn) || pawn->IsPlayerControlled() == false)
	{
		return;
	}

	if (_TriggerOnce && _HasTriggered)
	{
		return;
	}

	_HasTriggered = true;
	URulesHorrorStorySubsystem::Send(this, _EventName);
}
