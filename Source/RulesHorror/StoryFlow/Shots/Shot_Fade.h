// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "Shot_Fade.generated.h"

/*
 * 화면을 색(기본 검정)으로 덮거나 걷어 낸다. Alpha 1 = 완전히 덮음, 0 = 다 보임.
 * Duration이 0이면 바로 그 값으로 맞춘다. 끝난 값은 다음 Fade 전까지 유지된다(레벨이 바뀌면 풀림).
 */
UCLASS(meta = (DisplayName = "Fade"))
class RULESHORROR_API UShot_Fade : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	/* 시작 값. 0보다 작으면 지금 화면 상태에서 시작한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade", meta = (ClampMax = "1.0"))
	float _FromAlpha = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float _ToAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade", meta = (ClampMin = "0.0", Units = "s"))
	float _Duration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade")
	FLinearColor _Color = FLinearColor::Black;

	/* 끄면 페이드를 걸어 두고 바로 다음 Shot으로 넘어간다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade")
	bool _WaitForFinish = true;

	bool _IsFadeStarted = false;
	float _FadeStartTime = 0.0f;

protected:
	virtual void OnEnterShot_Implementation() override;
	virtual void OnTickShot_Implementation(float _delta_time) override;

	bool TryStartFade();
};
