// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_SiteBase.generated.h"


UCLASS(abstract)
class RULESHORROR_API UUI_SiteBase : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	FString _AdditionalSiteAddress = FString();

public:
	UFUNCTION(BlueprintCallable)
	void SetAdditionalSiteAddress(const FString& _new_address);

	const FString& GetAdditionalSiteAddress() const { return _AdditionalSiteAddress; }
};
