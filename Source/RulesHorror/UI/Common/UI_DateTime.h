// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_DateTime.generated.h"

/**
 * 초기 데이터 로딩 중
 */
UCLASS(abstract)
class RULESHORROR_API UUI_DateTime : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	bool _OverwriteDate = false;

	UPROPERTY(EditAnywhere)
	bool _OverwriteTime = false;

	UPROPERTY(EditAnywhere)
	FDateTime _OverwriteDateTime;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintCallable)
	void OverwriteDateTime(const FDateTime& _overwrite_date_time, bool _overwrite_date, bool _overwrite_time);

protected:
	UFUNCTION()
	void UpdateDateTime();

	UFUNCTION(BlueprintImplementableEvent, meta = (ForceAsFunction))
	void SetDate(int32 _year, int32 _month, int32 _day);

	UFUNCTION(BlueprintImplementableEvent, meta = (ForceAsFunction))
	void SetTime(int32 _hour, int32 _minute, bool _is_am);
};
