// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
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
	virtual void OnShow_Implementation() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetNickName(const FText& _nick_name);

	void ChangeSite(class UUI_SiteBase* _new_site);

	/* 지금 보이는 사이트 페이지의 주소를 주소창에 알린다. */
	void BroadcastOnSiteChangedEvent(UUI_SiteBase* _site);
};
