// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI/Lobby/WindowBase/UI_WindowLayout.h"
#include "WindowBase.generated.h"


UCLASS(abstract)
class RULESHORROR_API UWindowBase : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUI_WindowLayout> WindowLayout = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool _IsMaximized = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D _LastNormalPos = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D _NormalSize = FVector2D(600.0f, 600.0f);

protected:
	bool _ShouldDrag = false;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void SynchronizeProperties() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& _geo, const FFocusEvent& _focus_event) override;
	virtual void NativeOnFocusLost(const FFocusEvent& _focus_event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& _geo, const FPointerEvent& _mouse_event) override;
	virtual void NativeOnDragDetected(const FGeometry& _geo, const FPointerEvent& _mouse_event, UDragDropOperation*& _out_operation) override;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDM_OnWindowFocused, UWindowBase*, _window_widget, bool, _is_focused);

	UPROPERTY(BlueprintAssignable)
	FDM_OnWindowFocused _OnWindowFocusedEvent;

public:
	UFUNCTION(BlueprintCallable)
	void ExecuteCommand(EWindowCommand _command);

	void SetLastNormalPos(const FVector2D& _pos)
	{
		_LastNormalPos = _pos;
	}

protected:
	void SetMaximize(bool _is_maximized);

};
