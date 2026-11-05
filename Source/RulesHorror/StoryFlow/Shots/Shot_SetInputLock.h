// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "Shot_SetInputLock.generated.h"

/*
 * 플레이어 입력을 잠그거나 푼다 (이동, 시선, 상호작용). 바로 다음 Shot으로 넘어간다.
 * 레벨이 바뀌면 풀린 상태로 시작하므로, 이어하기 체크포인트 뒤에는 필요한 상태를 다시 건다.
 */
UCLASS(meta = (DisplayName = "Set Input Lock"))
class RULESHORROR_API UShot_SetInputLock : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	bool _Locked = true;

protected:
	virtual void OnEnterShot_Implementation() override;
};
