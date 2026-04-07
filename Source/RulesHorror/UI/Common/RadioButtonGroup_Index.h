// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/RadioButtonGroup.h"
#include "UI/Common/RadioButton_Index.h"
#include "RadioButtonGroup_Index.generated.h"


UCLASS(abstract)
class RULESHORROR_API URadioButtonGroup_Index : public URadioButtonGroup
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class URadioButton_Index> _IndexButtonClass = nullptr;

protected:
	UPROPERTY(EditAnywhere, meta  = (Tooltip = "한번에 보여질 수 있는 최대 button 수량"))
	int32 _MaxButtonCount = 5;

	UPROPERTY(meta = (Tooltip = "최대 인덱스 값"))
	int32 _MaxIndex = -1;

	UPROPERTY(meta = (Tooltip = "(CurrentIndex - 1) / MaxButtonCount"))
	int32 _PageIndex = -1;

	UPROPERTY(meta = (Tooltip = "현재 선택된 인덱스 값"))
	int32 _CurrentIndex = -1;

protected:
	virtual void SynchronizeProperties() override;

public:
	UFUNCTION(BlueprintCallable)
	void InitWidget(int32 _max_index, int32 _current_index);

	UFUNCTION(BlueprintCallable)
	void SetCurrentIndex(int32 _current_index);

protected:
	void UpdatePageButtons();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowLeftPageButton(bool _show);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowRightPageButton(bool _show);

	UFUNCTION(BlueprintCallable)
	void OnClickPageButton(bool _is_left);

public:
	UFUNCTION(BlueprintPure)
	int32 GetMaxButtonCount() const { return _MaxButtonCount; }

	UFUNCTION(BlueprintPure)
	int32 GetCurrentIndex() const { return _CurrentIndex; }

};
