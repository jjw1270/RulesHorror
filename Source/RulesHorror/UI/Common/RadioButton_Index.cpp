// Copyright (c) 2026 장윤제. All rights reserved.


#include "RadioButton_Index.h"
#include "Components/TextBlock.h"

void URadioButton_Index::SetIndex(int32 _new_index)
{
	_Index = _new_index;

	FNumberFormattingOptions option;
	option.AlwaysSign = false;

	// 유저한테 보여주는건 +1
	TXT_Index->SetText(FText::AsNumber(_Index + 1, &option));
}