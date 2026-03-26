// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Lobby/Explorer/UI_SiteBase.h"
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

	virtual const FString& GetAdditionalSiteAddress() override;

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

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetTitle(const FText& _text);
	
	UFUNCTION(BlueprintCallable)
	void PlayCurrentStory();
};
