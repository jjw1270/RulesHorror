// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "Item/RulesHorrorItemDefines.h"
#include "UI_MainSite.generated.h"


UCLASS(abstract)
class RULESHORROR_API UUI_MainSite : public UWidgetBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UUniformGridPanel> UGP_Story = nullptr;
	
protected:
	UPROPERTY(Transient)
	TArray<FStoryTableRow> _AllStroyItemRows;

	int32 _StoryTitleNum = 0;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetNickName(const FText& _nick_name);

	void UpdateStoryTitles();

#pragma region RadioButton
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class URadioButtonGroup_Index> RadioButtonGroup_Index = nullptr;

protected:
	static TOptional<int32> _LastRadioButtonIndex;

protected:
	UFUNCTION()
	void OnRadioButtonSelected(class URadioButton* _btn);

#pragma endregion RadioButton
};
