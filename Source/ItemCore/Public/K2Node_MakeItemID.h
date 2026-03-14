// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "K2Node.h"
#include "ItemID.h"
#include "K2Node_MakeItemID.generated.h"


UCLASS()
class ITEMCORE_API UK2Node_MakeItemID : public UK2Node
{
	GENERATED_BODY()

public:
	virtual bool IsNodePure() const override { return true; }

	virtual void AllocateDefaultPins() override;
	virtual void ExpandNode(class FKismetCompilerContext& _compiler_context, class UEdGraph* _source_graph) override;

	virtual void PinDefaultValueChanged(UEdGraphPin* _pin) override;
	virtual void PostReconstructNode() override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& _old_pins) override;

	virtual FText GetNodeTitle(ENodeTitleType::Type _title_type) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(class FBlueprintActionDatabaseRegistrar& _action_registrar) const override;
	
public:
	static FName GetTypePinName() { return TEXT("Type"); }
	static FName GetSubTypePinName() { return TEXT("SubType"); }
	static FName GetSerialPinName() { return TEXT("Serial"); }
	static FName GetReturnValuePinName() { return TEXT("ItemID"); }

protected:
	UPROPERTY()
	EItemType _CachedType = EItemType::NA;

protected:
	EItemType GetCurrentItemType() const;
	EItemType GetItemTypeFromPinDefault(const UEdGraphPin* _type_pin) const;
	void RefreshSubTypePin();
};
