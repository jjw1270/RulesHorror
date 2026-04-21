// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/ClickButton.h"
#include "UI/Office/MonitorScreen/WindowBase/WindowDefines.h"
#include "BTN_WindowIcon.generated.h"


UCLASS(abstract)
class RULESHORROR_API UBTN_WindowIcon : public UClickButton
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	EWindowWidgetType _WindowWidgetType = EWindowWidgetType::NA;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<class UTexture2D> _IconImage = nullptr;

	UPROPERTY(EditAnywhere)
	FText _IconText;

protected:
	virtual void SynchronizeProperties() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetIconImage(class UTexture2D* _image);

	UFUNCTION(BlueprintImplementableEvent)
	void SetIconText(const FText& _text);

public:
	UFUNCTION(BlueprintPure)
	EWindowWidgetType GetWindowWidgetType() const { return _WindowWidgetType; }
};
