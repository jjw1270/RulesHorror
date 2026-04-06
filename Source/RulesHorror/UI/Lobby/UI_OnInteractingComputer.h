// Fill out your copyright notice in the Description page of Project Settings.

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
