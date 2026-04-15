// Copyright (c) 2026 장윤제. All rights reserved.


#include "RadioButtonGroup_Index.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/StackBox.h"
#include "Components/StackBoxSlot.h"

void URadioButtonGroup_Index::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	auto hb_slot = Cast<UHorizontalBoxSlot>(StackBox->Slot);
	if (IsValid(hb_slot))
	{
		hb_slot->SetPadding(_ButtonPadding);
	}

	UpdatePageButtons();
}

void URadioButtonGroup_Index::InitWidget(int32 _max_index, int32 _current_index)
{
	_MaxIndex = _max_index;

	_CurrentIndex = -1;
	SetCurrentIndex(_current_index);
}

void URadioButtonGroup_Index::SetCurrentIndex(int32 _current_index)
{
	if(IsAny(_MaxIndex < 0, _MaxButtonCount < 0))
	{
		TRACE_ERROR(TEXT("Init Widget을 해야 합니다!"));
		return;
	}

	if (_current_index < 0 || _current_index > _MaxIndex)
	{
		TRACE_WARNING(TEXT("_current_index 가 범위 밖입니다. clamp 됩니다."));
		_current_index = FMath::Clamp(_current_index, 0, _MaxIndex);
	}

	if (_CurrentIndex == _current_index)
		return;

	_CurrentIndex = _current_index;
	_PageIndex = (_CurrentIndex > 0) ? _CurrentIndex / _MaxButtonCount : 0;

	if (IsInvalid(_IndexButtonClass))
		return;

	const int32 page_start_index = _PageIndex * _MaxButtonCount;
	const int32 button_count = FMath::Clamp((_MaxIndex - page_start_index) + 1, 0, _MaxButtonCount);

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

	int32 _idx = page_start_index;
	for (auto child : StackBox->GetAllChildren())
	{
		auto button = Cast<URadioButton_Index>(child);
		if (IsValid(button))
		{
			button->SetIndex(_idx);

			_idx++;
		}
	}

	UpdatePageButtons();

	SelectRadioButtonByIndex(_CurrentIndex % _MaxButtonCount);
}

void URadioButtonGroup_Index::UpdatePageButtons()
{
	ShowLeftPageButton(_PageIndex > 0);
	ShowRightPageButton(_PageIndex < (_MaxIndex - 1) / _MaxButtonCount);
}

void URadioButtonGroup_Index::OnClickPageButton(bool _is_left)
{
	if (_is_left)
	{
		// 제일 우측 버튼의 인덱스
		SetCurrentIndex(FMath::Max(0, _PageIndex * _MaxButtonCount - 1));
	}
	else
	{
		// 제일 좌측 버튼의 인덱스
		SetCurrentIndex((_PageIndex * _MaxButtonCount) + _MaxButtonCount);
	}
}
