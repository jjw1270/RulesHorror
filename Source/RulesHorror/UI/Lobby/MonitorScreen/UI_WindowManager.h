// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Lobby/MonitorScreen/UI_MonitorScreenWidget.h"
#include "UI/Lobby/MonitorScreen/WindowBase/WindowDefines.h"
#include "UI_WindowManager.generated.h"

class UCanvasPanel;
class UHorizontalBox;
class UTexture2D;
class UWindowBase;
class UBTN_WindowTab;
class UButtonBase;

USTRUCT(BlueprintType)
struct FWindowData
{
	GENERATED_BODY()

public:
	// Window widget
	UPROPERTY(EditAnywhere)
	TSubclassOf<UWindowBase> WindowWidgetClass = nullptr;

	UPROPERTY(EditAnywhere)
	bool IsMaximized = false;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UWindowBase> WindowWidget = nullptr;

public:
	// Window tab
	UPROPERTY(EditAnywhere)
	bool CreateTab = true;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UTexture2D> WindowTabIcon = nullptr;

	UPROPERTY(EditAnywhere)
	FText WindowTabText;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBTN_WindowTab> WindowTab = nullptr;

	UPROPERTY(Transient)
	bool HasBeenOpened = false;
};

UCLASS(Abstract)
class RULESHORROR_API UUI_WindowManager : public UUI_MonitorScreenWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CP_Window = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HB_WindowTab = nullptr;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UBTN_WindowTab> _WindowTabClass = nullptr;

	UPROPERTY(EditAnywhere)
	TMap<EWindowWidgetType, FWindowData> _WindowDataMap;

	FVector2D _InitialWindowPos = FVector2D(80.0f, 80.0f);
	FVector2D _InitialNormalPos = FVector2D(120.0f, 120.0f);
	FVector2D _WindowOffset = FVector2D(24.0f, 24.0f);
	FVector2D _DefaultWindowSize = FVector2D(600.0f, 600.0f);

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

	UFUNCTION()
	void OnClickWindowTab(UButtonBase* _tab_button);

	UFUNCTION()
	void OnWindowFocused(UWindowBase* _focused_window_widget, bool _is_focused);

public:
	UWindowBase* GetTopWindow() const;

private:
	FVector2D GetNextWindowPosition() const;
	void InitializeWindowWidget(UWindowBase* _window_widget, EWindowWidgetType _type, const FVector2D& _window_pos);
	void AttachWindowToCanvas(UWindowBase* _window_widget);
	void CreateWindowTab(EWindowWidgetType _type, FWindowData& _window_data);
	int32 GetMaxWindowZOrder() const;

};