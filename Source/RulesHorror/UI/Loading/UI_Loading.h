// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_Loading.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_Loading : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TObjectPtr<class UStoryFlowSubsystem> _StoryFlowSubsystem = nullptr;

	UPROPERTY(BlueprintReadOnly)
	float _ProgressRate = -1.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& _geo, float _delta) override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnUpdateProgressRate();
};
