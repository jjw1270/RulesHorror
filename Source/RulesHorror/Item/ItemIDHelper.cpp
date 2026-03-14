// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemIDHelper.h"

FItemID UItemIDHelper::MakeItemID(EItemType _type, uint8 _sub_type, int32 _serial)
{
	_serial = FMath::Clamp(_serial, 0, 9999);

	return FItemID(_type, _sub_type, static_cast<uint16>(_serial));
}

void UItemIDHelper::BreakItemID(FItemID _item_id, EItemType& _type, uint8& _sub_type, int32& _serial)
{
	_type = _item_id.GetType();
	_sub_type = _item_id.GetSubType();
	_serial = _item_id.GetSerial();
}

EItemType UItemIDHelper::GetType(FItemID _item_id)
{
	return _item_id.GetType();
}

uint8 UItemIDHelper::GetSubType(FItemID _item_id)
{
	return _item_id.GetSubType();
}

int32 UItemIDHelper::GetSerial(FItemID _item_id)
{
	return _item_id.GetSerial();
}

int32 UItemIDHelper::ToInteger(FItemID _item_id)
{
	return _item_id;
}

FString UItemIDHelper::ToString(FItemID _item_id)
{
	return _item_id.ToString();
}

bool UItemIDHelper::IsValid(FItemID _item_id, FString& _out_reason)
{
	const FItemIDValidationResult result = _item_id.Validate();
	_out_reason = result._Reason;

	return result.IsValid();
}
