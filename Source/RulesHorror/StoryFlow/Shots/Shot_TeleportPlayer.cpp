// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_TeleportPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "CommonUtils.h"

void UShot_TeleportPlayer::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	TArray<AActor*> targets;
	UGameplayStatics::GetAllActorsWithTag(this, _TargetTag, targets);
	APawn* pawn = UGameplayStatics::GetPlayerPawn(this, 0);

	if (targets.Num() == 0 || IsInvalid(targets[0]) || IsInvalid(pawn))
	{
		TRACE_WARNING(TEXT("Teleport target '%s' or player pawn not found."), *_TargetTag.ToString());
		FinishShot();
		return;
	}

	const AActor* target = targets[0];
	const float distance = FVector::Dist(pawn->GetActorLocation(), target->GetActorLocation());
	if (_OnlyIfFartherThan <= 0.0f || distance > _OnlyIfFartherThan)
	{
		pawn->TeleportTo(target->GetActorLocation(), target->GetActorRotation(), false, true);

		if (AController* controller = pawn->GetController())
		{
			controller->SetControlRotation(target->GetActorRotation());
		}
	}

	FinishShot();
}
