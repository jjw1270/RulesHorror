// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Office/MonitorScreen/Explorer/UI_SiteBase.h"
#include "Item/RulesHorrorItemIDs.h"
#include "Site_StoryDetail.generated.h"


UCLASS(abstract)
class RULESHORROR_API USite_StoryDetail : public UUI_SiteBase
{
	GENERATED_BODY()

protected:
	static FItemID_Story _LastStoryID;
	FItemID_Story _CurrentStoryID;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

#pragma region GoToStoryList
protected:
	UPROPERTY(BlueprintReadOnly , meta = (BindWidget))
	TObjectPtr<class UClickButton> BTN_GoToStoryList = nullptr;

public:
	DECLARE_DELEGATE(FD_OnClickShowStoryList);
	FD_OnClickShowStoryList _OnClickShowStoryListEvent;

protected:
	UFUNCTION()
	void OnClickGoToStoryListButton(class UButtonBase* _btn);

#pragma endregion GoToStoryList

public:
	void SetStoryID(FItemID_Story _story_id);

	/* 마지막으로 본 괴담을 잊는다. 새 게임/이어하기 때 부른다 (지난 판 페이지가 남지 않게). */
	static void ResetLastStoryID() { _LastStoryID = FItemID_Story::Zero; }

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetTitle(const FText& _text);
	
	UFUNCTION(BlueprintCallable)
	void PlayCurrentStory();
};
