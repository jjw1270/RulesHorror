// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_Loading.generated.h"

/**
 * 초기 데이터 로딩 중
 */
UCLASS(abstract)
class RULESHORROR_API UUI_Loading : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, meta = (Tooltip="최소 로딩 시간 범위"))
	FVector2D _MinLoadingTimeRange = FVector2D(3.0f, 6.0f);

	float _MinLoadingTime = 0.0f;

	float _LoadingTimer = 0.0f;

protected:
	virtual void OnShow_Implementation() override;
	virtual void NativeTick(const FGeometry& _geo, float _delta) override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadingFinished();
};
