// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PageBase.h"
#include "Page_InitUser.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UPage_InitUser : public UPageBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	FString _CurrentNickname;

protected:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ShowSetNewNickname(bool _show);

	UFUNCTION(BlueprintCallable)
	bool SetNewNickname(const FString& _new_nickname, FText& _out_error_text);

};
