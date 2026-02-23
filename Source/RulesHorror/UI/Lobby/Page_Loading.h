// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PageBase.h"
#include "Page_Loading.generated.h"

/**
 * 초기 데이터 로딩 중
 */
UCLASS(abstract)
class RULESHORROR_API UPage_Loading : public UPageBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, meta = (Tooltip="최소 로딩 시간 범위"))
	FVector2D _MinLoadingTimeRange = FVector2D(3.0f, 6.0f);

	float _MinLoadingTime = 0.0f;

	bool _IsOnLoading = false;

	TOptional<FDateTime> _LoadingStartDateTime;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UPageBase> _LoadingFinishedPageClass = nullptr;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& _geo, float _delta) override;

protected:
	void StartLoading();

	void OnLoadingFinished();
};
