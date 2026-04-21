// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_OptionPage.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_OptionPage : public UWidgetBase
{
	GENERATED_BODY()
	
protected:
	virtual void OnShow_Implementation() override;
	virtual void OnHide_Implementation(EWidgetHideType _hide_type) override;

};
