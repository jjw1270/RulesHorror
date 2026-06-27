// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemID.h"
#include "ItemIDHelper.generated.h"

UCLASS()
class ITEMCORE_API UItemIDHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item", meta = (BlueprintInternalUseOnly = "true", ToolTip = "Type/SubType/Serial 값으로 ItemID를 생성"))
	static FItemID MakeItemID(EItemType _type, uint8 _sub_type, int32 _serial);

	UFUNCTION(BlueprintPure, Category = "Item", meta = (ToolTip = "ItemID를 Type/SubType/Serial 값으로 분해"))
	static void BreakItemID(FItemID _item_id, EItemType& _type, uint8& _sub_type, int32& _serial);

	UFUNCTION(BlueprintPure, Category = "Item", meta = (ToolTip = "ItemID의 Type 값을 반환"))
	static EItemType GetType(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "Item", meta = (ToolTip = "ItemID의 SubType 값을 반환"))
	static uint8 GetSubType(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "Item", meta = (ToolTip = "ItemID의 Serial 값을 반환"))
	static int32 GetSerial(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "Item", meta = (ToolTip = "ItemID의 내부 정수 값을 반환"))
	static int32 ToInteger(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "Item", meta = (ToolTip = "ItemID의 내부 정수 값을 문자열로 반환"))
	static FString ToString(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "Item", meta = (ToolTip = "ItemID 유효성을 검사하고 실패 또는 성공 사유를 반환"))
	static bool IsValid(FItemID _item_id, FString& _out_reason);
};
