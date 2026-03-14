// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUtils.h"
#include "ItemDefines.h"
#include "ItemID.generated.h"

/*
* Type 3자리 + SubType 3자리 + Serial 4자리
* 000 000 0000
* bit 패킹 아님!! 그냥 10진수
*/
USTRUCT(BlueprintType)
struct ITEMCORE_API FItemID
{
	GENERATED_BODY()

	friend class FItemIDCustomization;

private:
	static constexpr uint32 TypeMultiplier = 10000000;
	static constexpr uint32 SubTypeMultiplier = 10000;
	static constexpr uint32 MaxTypeValue = 255;
	static constexpr uint32 MaxSubTypeValue = 255;
	static constexpr uint32 MaxSerialValue = 9999;

protected:
	UPROPERTY(EditAnywhere)
	uint32 _Value = 0;

public:
	FItemID() = default;

	FItemID(uint32 _value)
	{
		_Value = _value;
	}

	FItemID(EItemType _type, uint8 _sub_type, uint16 _serial)
	{
		Set(_type, _sub_type, _serial);
	}

	friend uint32 GetTypeHash(const FItemID& _item_id)
	{
		return GetTypeHash(_item_id._Value);
	}

	friend bool operator==(const FItemID& _left, uint32 _right)
	{
		return _left._Value == _right;
	}

	friend bool operator!=(const FItemID& _left, uint32 _right)
	{
		return _left._Value != _right;
	}

	friend bool operator==(const FItemID& _left, const FItemID& _right)
	{
		return _left._Value == _right._Value;
	}

	friend bool operator!=(const FItemID& _left, const FItemID& _right)
	{
		return _left._Value != _right._Value;
	}

	operator uint32() const
	{
		return _Value;
	}

	FString ToString() const
	{
		return FString::FromInt(_Value);
	}

public:
	void Set(EItemType _type, uint8 _sub_type, uint16 _serial);

	void SetType(EItemType _type)
	{
		// Subtype, Serial 값 Invalidate
		Set(_type, 0, 0);
	}

	void SetSubType(uint8 _sub_type)
	{
		// Serial 값 Invalidate
		Set(GetType(), _sub_type, 0);
	}

	void SetSerial(uint16 _serial)
	{
		Set(GetType(), GetSubType(), _serial);
	}

public:
	EItemType GetType() const
	{
		return static_cast<EItemType>(_Value / TypeMultiplier);
	}

	uint8 GetSubType() const
	{
		return static_cast<uint8>((_Value / SubTypeMultiplier) % 1000);
	}

	static const UEnum* GetSubTypeEnum(EItemType _type);

	uint16 GetSerial() const
	{
		return static_cast<uint16>(_Value % SubTypeMultiplier);
	}

public:
	FItemIDValidationResult Validate() const;

	bool IsValid() const
	{
		return Validate().IsValid();
	}
};
