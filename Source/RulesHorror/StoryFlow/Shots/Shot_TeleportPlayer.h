// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "Shot_TeleportPlayer.generated.h"

/*
 * 플레이어를 태그 붙은 액터(빈 액터나 TargetPoint)의 위치와 방향으로 옮긴다. 바로 다음 Shot으로 넘어간다.
 * 이어하기로 레벨을 처음부터 열었을 때 플레이어를 그 장면 위치로 되돌리는 용도.
 */
UCLASS(meta = (DisplayName = "Teleport Player"))
class RULESHORROR_API UShot_TeleportPlayer : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport")
	FName _TargetTag;

	/* 0보다 크면 목표와 이 거리(cm) 이상 떨어져 있을 때만 옮긴다 (이미 그 근처면 그대로 둔다). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0.0", Units = "cm"))
	float _OnlyIfFartherThan = 0.0f;

protected:
	virtual void OnEnterShot_Implementation() override;
};
