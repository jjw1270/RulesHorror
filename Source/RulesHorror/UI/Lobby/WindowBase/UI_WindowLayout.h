// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "UI_WindowLayout.generated.h"

UENUM(BlueprintType)
enum class EWindowCommand : uint8
{
	Minimize						UMETA(ToolTip = "최소화"),
	RestoreSize				UMETA(ToolTip = "사이즈 토글"),
	Close							UMETA(ToolTip = "닫기"),
	StartDrag					UMETA(ToolTip = "창 이동"),
};

UCLASS(abstract)
class RULESHORROR_API UUI_WindowLayout : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> _TitleImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText _TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool _EnableTitleButtons = true;

public:
	DECLARE_DELEGATE_OneParam(F_OnRequestCommand, EWindowCommand);

	F_OnRequestCommand _OnRequestCommandEvent;

protected:
	UFUNCTION(BlueprintCallable)
	void RequestCommand(EWindowCommand _command);
};
