// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PageBase.h"
#include "UI/Lobby/WindowBase/WindowDefines.h"
#include "Page_MainLobby.generated.h"


class UWindowBase;
class UBTN_WindowTab;

USTRUCT(BlueprintType)
struct FWindowData
{
	GENERATED_BODY()

public:
	// window widget
	UPROPERTY(EditAnywhere)
	TSubclassOf<UWindowBase> WindowWidgetClass = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UWindowBase> WindowWidget = nullptr;

public:
	// window tab
	UPROPERTY(EditAnywhere)
	TObjectPtr<UTexture2D> WindowTabIcon = nullptr;

	UPROPERTY(EditAnywhere)
	FText WindowTabText;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBTN_WindowTab> WindowTab = nullptr;
};

UCLASS(abstract)
class RULESHORROR_API UPage_MainLobby : public UPageBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> CP_Window = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UHorizontalBox> HB_WindowTab = nullptr;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UBTN_WindowTab> _WindowTabClass = nullptr;

	UPROPERTY(EditAnywhere)
	TMap<EWindowWidgetType, FWindowData> _WindowDataMap;

protected:
	virtual void NativeOnInitialized() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetBackgroundImage(const TSoftObjectPtr<UTexture2D>& _image);

protected:
	UFUNCTION(BlueprintCallable)
	void CreateWindow(EWindowWidgetType _type);

	UFUNCTION(BlueprintCallable)
	void OpenWindow(EWindowWidgetType _type, bool _is_open);

	void SetTopWindow(UWindowBase* _target_window);
	void UpdateTopWindow();
	UWindowBase* GetTopWindow() const;

	UFUNCTION()
	void OnClickWindowTab(class UButtonBase* _tab_button);

	UFUNCTION()
	void OnWindowFocused(UWindowBase* _focused_window_widget, bool _is_focused);

};
