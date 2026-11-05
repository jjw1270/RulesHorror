// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "Shot_SetObjective.generated.h"

/* 왼쪽 위 목표 문구를 바꾼다. 비워 두면 숨긴다. 바로 다음 Shot으로 넘어간다. */
UCLASS(meta = (DisplayName = "Set Objective"))
class RULESHORROR_API UShot_SetObjective : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FText _Objective;

protected:
	virtual void OnEnterShot_Implementation() override;
};
