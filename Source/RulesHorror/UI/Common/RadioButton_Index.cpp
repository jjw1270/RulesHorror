// Fill out your copyright notice in the Description page of Project Settings.


#include "RadioButton_Index.h"
#include "Components/TextBlock.h"

void URadioButton_Index::SetIndex(int32 _new_index)
{
	_Index = _new_index;

	FNumberFormattingOptions option;
	option.AlwaysSign = false;

	TXT_Index->SetText(FText::AsNumber(_Index, &option));
}