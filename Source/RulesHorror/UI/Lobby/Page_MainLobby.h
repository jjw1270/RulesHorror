// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PageBase.h"
#include "Page_MainLobby.generated.h"

class UWindowBase;
class UClickButton;
class UWindow_Explorer;
class UWindow_ControlPanel;
/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UPage_MainLobby : public UPageBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> CP_Windows = nullptr;

// Explorer
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWindow_Explorer> Window_Explorer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UClickButton> BTN_ExplorerTab = nullptr;

// Control Panel
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWindow_ControlPanel> Window_ControlPanel = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UClickButton> BTN_ControlPanelTab = nullptr;

protected:
	virtual void NativeOnInitialized() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetBackgroundImage(const TSoftObjectPtr<UTexture2D>& _image);

protected:
	UFUNCTION()
	void OnWindowFocused(UWindowBase* _focused_window_widget, bool _is_focused);

	UFUNCTION()
	void OnClickWindowTabButton(class UButtonBase* _tab_button);

};
