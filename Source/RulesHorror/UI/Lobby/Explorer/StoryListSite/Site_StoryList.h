// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Lobby/Explorer/UI_SiteBase.h"
#include "Item/RulesHorrorItemDefines.h"
#include "Site_StoryList.generated.h"


UCLASS(abstract)
class RULESHORROR_API USite_StoryList : public UUI_SiteBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<class UUniformGridPanel> UniformGridPanel = nullptr;
	
protected:
	UPROPERTY(Transient)
	TArray<FStoryTableRow> _AllStroyItemRows;

	int32 _StoryTitleNum = 0;

public:
	DECLARE_DELEGATE_OneParam(FD_OnClickShowStoryDetail, const FItemID_Story&);

	FD_OnClickShowStoryDetail _OnClickShowStoryDetailEvent;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

protected:
	void UpdateStoryTitles();

	UFUNCTION()
	void OnClickStoryTitleButton(class UButtonBase* _btn);

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
