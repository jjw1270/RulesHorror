// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Lobby/WindowBase/WindowBase.h"
#include "Window_Explorer.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UWindow_Explorer : public UWindowBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UUI_SitePanel> UI_SitePanel = nullptr;

protected:
	virtual void NativeOnInitialized() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetAddressText(const FString& _text);

};
