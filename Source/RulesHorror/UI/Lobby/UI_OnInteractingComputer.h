// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBase.h"
#include "UI_OnInteractingComputer.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_OnInteractingComputer : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class AComputer> _Computer = nullptr;

public:
	void SetComputer(AComputer* _new_computer)
	{
		_Computer = _new_computer;
	}
};
