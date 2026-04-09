// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/ClickButton.h"
#include "UI/Lobby/MonitorScreen/WindowBase/WindowDefines.h"
#include "BTN_WindowTab.generated.h"


UCLASS(abstract)
class RULESHORROR_API UBTN_WindowTab : public UClickButton
{
	GENERATED_BODY()

public:
	EWindowWidgetType _WindowWidgetType = EWindowWidgetType::NA;

protected:
	UPROPERTY(EditAnywhere)
	TMap<EButtonState, FButtonStyleConfig> _HighlightedStateStyles;

	UPROPERTY()
	TMap<EButtonState, FButtonStyleConfig> _NonHighlightedStateStyles;

protected:
	virtual void NativeOnInitialized() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetTabIcon(UTexture2D* _image);
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetTabText(const FText& _text);

	void SetHighlight(bool _is_highlighted);
};
