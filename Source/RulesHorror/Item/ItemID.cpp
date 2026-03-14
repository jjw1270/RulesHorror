// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemID.h"

void FItemID::Set(EItemType _type, uint8 _sub_type, uint16 _serial)
{
	const uint32 type_value = static_cast<uint32>(_type);
	const uint32 sub_type_value = static_cast<uint32>(_sub_type);
	const uint32 serial_value = static_cast<uint32>(_serial);

	if (type_value > MaxTypeValue)
	{
		TRACE_ERROR(TEXT("Type 값은 %d 이하여야 합니다."), MaxTypeValue);
		_Value = 0;
		return;
	}

	if (sub_type_value > MaxSubTypeValue)
	{
		TRACE_ERROR(TEXT("SubType 값은 %d 이하여야 합니다."), MaxSubTypeValue);
		_Value = 0;
		return;
	}

	if (serial_value > MaxSerialValue)
	{
		TRACE_ERROR(TEXT("시리얼 값은 %d 이하여야 합니다."), MaxSerialValue);
		_Value = 0;
		return;
	}

	_Value = type_value * TypeMultiplier + sub_type_value * SubTypeMultiplier + serial_value;
}

const UEnum* FItemID::GetSubTypeEnum(EItemType _type)
{
	switch (_type)
	{
	case EItemType::NA:
		return nullptr;

	default:
		return StaticEnum<EDefaultItemSubType>();
	}
}

FItemIDValidationResult FItemID::Validate() const
{
	// type check
	const EItemType type = GetType();

	if (!IsValidEnumValue(type))
	{
		return { false, TEXT("Type 값이 유효한 enum 값이 아닙니다.") };
	}

	if (type == EItemType::NA)
	{
		return { false, TEXT("Type 값이 NA 입니다.") };
	}

	if (type == EItemType::MAX)
	{
		return { false, TEXT("Type 값이 MAX 입니다.") };
	}

	// sub type check
	const uint8 sub_type = GetSubType();

	if (sub_type == 0)
	{
		return { false, TEXT("SubType 값이 0 입니다.") };
	}

	const UEnum* sub_type_enum = GetSubTypeEnum(type);
	if (sub_type_enum == nullptr)
	{
		return { false, TEXT("SubType Enum 이 nullptr 입니다.") };
	}

	if (!IsValidEnumValue(sub_type_enum, sub_type))
	{
		return { false, FString::Printf(TEXT("SubType 값 %d 가 현재 Type 에 대한 유효한 enum 값이 아닙니다."), sub_type) };
	}

	// serial check
	const uint16 serial = GetSerial();
	if (serial == 0)
	{
		return { false, FString::Printf(TEXT("Serial 값이 0 입니다.")) };
	}
	
	if (serial > MaxSerialValue)
	{
		return { false, FString::Printf(TEXT("Serial 값 %d 가 최대값 %d 를 초과했습니다."), serial, MaxSerialValue) };
	}

	// success!
	return { true, TEXT("Success") };
}
