// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_MainLobby.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_MainLobby : public UWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
protected:
	UFUNCTION(BlueprintCallable)
	void OnClick_StartNewGame();

	UFUNCTION(BlueprintCallable)
	void OnClick_StartSavedGame();

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetCanStartSavedGame(bool _value);

};
