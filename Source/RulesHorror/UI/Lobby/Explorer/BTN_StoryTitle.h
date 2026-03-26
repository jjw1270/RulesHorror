// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/ClickButton.h"
#include "Item/RulesHorrorItemIDs.h"
#include "BTN_StoryTitle.generated.h"


UCLASS(abstract)
class RULESHORROR_API UBTN_StoryTitle : public UClickButton
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

public:
	UFUNCTION(BlueprintPure)
	const FItemID_Story& GetStroyID() const { return _StoryID; }
};
