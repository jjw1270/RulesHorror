// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/RadioButtonGroup.h"
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
	int32 _MaxIndex = 0;

	UPROPERTY(meta = (Tooltip = "현재 보여지는 버튼들 중 첫번째 버튼의 인덱스 값"))
	int32 _CurrentIndex = -1;

protected:
	virtual void SynchronizeProperties() override;

public:
	UFUNCTION(BlueprintCallable)
	void SetMaxIndex(int32 _max_index);

	UFUNCTION(BlueprintCallable)
	void SetCurrentIndex(int32 _current_index);

protected:
	void UpdateLeftRightButtons();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowLeftButton(bool _show);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowRightButton(bool _show);

	UFUNCTION(BlueprintCallable)
	void OnClickLeftButton();

	UFUNCTION(BlueprintCallable)
	void OnClickRightButton();
};
