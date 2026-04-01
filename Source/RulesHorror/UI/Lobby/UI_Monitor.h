// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBase.h"
#include "UI_Monitor.generated.h"

/**
 * 
 */
UCLASS(abstract)
class RULESHORROR_API UUI_Monitor : public UWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void OnShow_Implementation() override;

#pragma region MonitorWidget
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidgetSwitcher> WidgetSwitcher = nullptr;
	
protected:
	static TOptional<int32> _LastActiveWidgetIndex;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere)
	int32 _DEBUG_Start_Widget_Index = -1;
#endif

#if WITH_EDITOR
public:
	static void ResetLastActiveWidgetindex() { _LastActiveWidgetIndex.Reset(); }
#endif

protected:
	UFUNCTION(BlueprintCallable)
	void SetActiveWidgetAndShow(UWidgetBase* _widget, bool _is_skip_anim);

#pragma endregion MonitorWidget
////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Cursor
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UUI_Cursor> UI_Cursor = nullptr;

public:
	UFUNCTION(BlueprintCallable)
	void ShowMonitorCursor(bool _is_show);

	void SetMonitorCursorPosition(const FVector2D& _pos);

#pragma endregion Cursor
};
