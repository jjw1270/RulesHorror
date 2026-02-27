// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PageBase.h"
#include "Page_MainLobby.generated.h"

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

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetBackgroundImage(const TSoftObjectPtr<UTexture2D>& _image);

#pragma region WindowExplorer
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWindow_Explorer> Window_Explorer = nullptr;

protected:
	UFUNCTION(BlueprintCallable)
	void ToggleWindowExplorer();

#pragma endregion WindowExplorer
////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region WindowControlPanel
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWindow_ControlPanel> Window_ControlPanel = nullptr;

public:
	UFUNCTION(BlueprintCallable)
	void ToggleWindowControlPanel();

#pragma endregion WindowControlPanel
////////////////////////////////////////////////////////////////////////////////////////////////
protected:
	UFUNCTION()
	void OnWindowFocused(class UWindowBase* _window_widget, bool _is_focused);
};
