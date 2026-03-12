// Fill out your copyright notice in the Description page of Project Settings.


#include "RadioButtonGroup_Index.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/StackBox.h"
#include "Components/StackBoxSlot.h"
#include "UI/Common/RadioButton_Index.h"

void URadioButtonGroup_Index::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	auto hb_slot = Cast<UHorizontalBoxSlot>(StackBox->Slot);
	if (IsValid(hb_slot))
	{
		hb_slot->SetPadding(_ButtonPadding);
	}

	UpdateLeftRightButtons();
}

void URadioButtonGroup_Index::SetMaxIndex(int32 _max_index)
{
	_MaxIndex = _max_index;

	UpdateLeftRightButtons();
}

void URadioButtonGroup_Index::SetCurrentIndex(int32 _current_index)
{
	if (_CurrentIndex == _current_index)
		return;
	_CurrentIndex = _current_index;

	if (IsInvalid(_IndexButtonClass))
		return;

	const int32 button_count = FMath::Min(_MaxButtonCount, _MaxIndex - _CurrentIndex);

	while (StackBox->GetChildrenCount() != button_count)
	{
		const int32 children_count = StackBox->GetChildrenCount();

		if (children_count > button_count)
		{
			StackBox->RemoveChildAt(children_count - 1);
		}
		else
		{
			auto button = CreateWidget<URadioButton_Index>(this, _IndexButtonClass);
			if (IsValid(button))
			{
				FString button_widget_id_str = FString::Printf(TEXT("BTN_%d"), children_count);
				button->SetWidgetID(FName(button_widget_id_str));

				StackBox->AddChildToStackBox(button);
			}
		}
	}

	UpdateRadioButtons();

	int32 _idx = _CurrentIndex + 1;
	for (auto child : StackBox->GetAllChildren())
	{
		auto button = Cast<URadioButton_Index>(child);
		if (IsValid(button))
		{
			button->SetIndex(_idx);

			_idx++;
		}
	}

	UpdateLeftRightButtons();
}

void URadioButtonGroup_Index::UpdateLeftRightButtons()
{
	ShowLeftButton(_CurrentIndex > 0);
	ShowRightButton(_CurrentIndex + _MaxButtonCount < _MaxIndex);
}

void URadioButtonGroup_Index::OnClickLeftButton()
{
	SetCurrentIndex(FMath::Max(0, _CurrentIndex - _MaxButtonCount));

	SelectRadioButtonByIndex(_MaxButtonCount-1);
}

void URadioButtonGroup_Index::OnClickRightButton()
{
	SetCurrentIndex(_CurrentIndex + _MaxButtonCount);
	SelectRadioButtonByIndex(0);
}
