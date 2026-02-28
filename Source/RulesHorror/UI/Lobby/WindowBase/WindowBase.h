// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI/Lobby/WindowBase/WindowDefines.h"
#include "WindowBase.generated.h"


UCLASS(abstract)
class RULESHORROR_API UWindowBase : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UUI_WindowLayout> WindowLayout = nullptr;

public:
	EWindowWidgetType _WindowWidgetType = EWindowWidgetType::NA;

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
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& _geo, const FPointerEvent& _mouse_event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& _geo, const FPointerEvent& _mouse_event) override;
	virtual void NativeOnDragDetected(const FGeometry& _geo, const FPointerEvent& _mouse_event, UDragDropOperation*& _out_operation) override;
	virtual bool NativeOnDrop(const FGeometry& _geo, const FDragDropEvent& _drag_drop_event, UDragDropOperation* _operation) override;

public:
	UFUNCTION(BlueprintCallable)
	void ExecuteCommand(EWindowCommand _command);

	void SetLastNormalPos(const FVector2D& _pos)
	{
		_LastNormalPos = _pos;
	}

public:
	void SetMaximize(bool _is_maximized);
	bool IsMaximized() const { return _IsMaximized; }

#pragma region WindowFocus
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDM_OnWindowFocused, UWindowBase*, _window_widget, bool, _is_focused);

	UPROPERTY(BlueprintAssignable)
	FDM_OnWindowFocused _OnWindowFocusedEvent;

public:
	void SetWindowFocused(bool _is_focused);

protected:
	void OnWindowFocused(bool _is_focused);
#pragma endregion WindowFocus
};
