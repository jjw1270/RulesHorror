// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBase.h"
#include "UI_InitUser.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_InitUser : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	FString _CurrentNickname;

protected:
	virtual void OnShow_Implementation() override;

protected:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ShowSetNewNickname(bool _show);

	UFUNCTION(BlueprintCallable)
	bool SetNewNickname(const FString& _new_nickname, FText& _out_error_text);

	bool CheckNickname(const FString& _nickname, FText& _out_error_text) const;
};
