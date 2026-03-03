// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/ClickButton.h"
#include "UI/Lobby/WindowBase/WindowDefines.h"
#include "BTN_WindowScaler.generated.h"


UCLASS(abstract)
class RULESHORROR_API UBTN_WindowScaler : public UClickButton
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<class USpacer> Spacer = nullptr;

protected:
	UPROPERTY(EditAnywhere)
	EWindowCommand _WindowCommandType = EWindowCommand::NA;

	UPROPERTY(EditAnywhere)
	FVector2D _SpacerSize = FVector2D::ZeroVector;

protected:
	virtual void SynchronizeProperties() override;
	virtual void OnButtonStateChanged_Implementation() override;

	void UpdateCursor(bool _show_scaler_cursor);

public:
	UFUNCTION(BlueprintPure)
	EWindowCommand GetWindowCommand() const { return _WindowCommandType; }
};
