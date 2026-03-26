// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "Item/RulesHorrorItemIDs.h"
#include "UI_SitePanel.generated.h"


UCLASS(abstract)
class RULESHORROR_API UUI_SitePanel : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UWidgetSwitcher> WS_Site = nullptr;

protected:
	UPROPERTY(EditAnywhere)
	FString _MainSiteAddress;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDM_OnSiteChanged, const FString&, _address);

	UPROPERTY(BlueprintCallable)
	FDM_OnSiteChanged _OnSiteChangedEvent;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetNickName(const FText& _nick_name);

	void ChangeSite(class UUI_SiteBase* _new_site);

#pragma region Site_StoryList
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class USite_StoryList> Site_StoryList = nullptr;

protected:
	UFUNCTION()
	void ShowStoryListSite();

#pragma endregion Site_StoryList
//////////////////////////////////////////////////////////////////////
#pragma region Site_StoryDetail
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class USite_StoryDetail> Site_StoryDetail = nullptr;

protected:
	UFUNCTION()
	void ShowStoryDetailSite(const FItemID_Story& _story_id);

#pragma endregion Site_StoryDetail

};
