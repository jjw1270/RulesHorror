// Copyright (c) 2026 장윤제. All rights reserved.

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
	const FItemID_Story& GetStoryID() const { return _StoryID; }
};
