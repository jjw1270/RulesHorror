// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "Item/RulesHorrorItemIDs.h"
#include "UI_StoryTitle.generated.h"


UCLASS(abstract)
class RULESHORROR_API UUI_StoryTitle : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	FItemID_Story _StoryID;

public:
	void SetStoryID(FItemID_Story _story_id);

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetTitle(const FText& _text);

};
