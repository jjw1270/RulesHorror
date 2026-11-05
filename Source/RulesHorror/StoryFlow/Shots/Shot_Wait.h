// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "Shot_Wait.generated.h"

/* 정한 시간만큼 기다린 뒤 다음 Shot으로 넘어간다. */
UCLASS(meta = (DisplayName = "Wait"))
class RULESHORROR_API UShot_Wait : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wait", meta = (ClampMin = "0.0", Units = "s"))
	float _Seconds = 1.0f;

protected:
	virtual void OnEnterShot_Implementation() override;
	virtual void OnTickShot_Implementation(float _delta_time) override;
};
