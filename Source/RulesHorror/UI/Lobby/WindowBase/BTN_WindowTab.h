// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/ClickButton.h"
#include "UI/Lobby/WindowBase/WindowDefines.h"
#include "BTN_WindowTab.generated.h"


UCLASS(abstract)
class RULESHORROR_API UBTN_WindowTab : public UClickButton
{
	GENERATED_BODY()

public:
	EWindowWidgetType _WindowWidgetType = EWindowWidgetType::NA;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetTabIcon(UTexture2D* _image);
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetTabText(const FText& _text);

	UFUNCTION(BlueprintImplementableEvent)
	void SetHighlight(bool _is_highlight);
};
