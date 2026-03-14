#include "K2Node_MakeItemID.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"

#include "ItemIDHelper.h"

#define LOCTEXT_NAMESPACE "K2Node_MakeItemID"

void UK2Node_MakeItemID::AllocateDefaultPins()
{
	const auto* schema = GetDefault<UEdGraphSchema_K2>();

	// Type : EItemType
	UEdGraphPin* type_pin = CreatePin(EGPD_Input, schema->PC_Byte, StaticEnum<EItemType>(), GetTypePinName());
	if (IsValid(type_pin))
	{
		type_pin->DefaultValue = TEnumToString(_CachedType);
	}

	// SubType
	// Type 기준으로 SubType enum 결정
	const UEnum* sub_type_enum = FItemID::GetSubTypeEnum(_CachedType);
	UEdGraphPin* sub_type_pin = nullptr;

	if (IsValid(sub_type_enum))
	{
		sub_type_pin = CreatePin(EGPD_Input, schema->PC_Byte, const_cast<UEnum*>(sub_type_enum), GetSubTypePinName());
	}
	else
	{
		sub_type_pin = CreatePin(EGPD_Input, schema->PC_Byte, GetSubTypePinName());
	}

	if (IsValid(sub_type_pin))
	{
		sub_type_pin->DefaultValue = TEXT("0");
	}
	
	// Serial : Int
	UEdGraphPin* serial_pin = CreatePin(EGPD_Input, schema->PC_Int, GetSerialPinName());
	if (IsValid(serial_pin))
	{
		serial_pin->DefaultValue = TEXT("0");
	}
	
	// Return : FItemID
	CreatePin(EGPD_Output, schema->PC_Struct, FItemID::StaticStruct(), GetReturnValuePinName());
}

void UK2Node_MakeItemID::ExpandNode(FKismetCompilerContext& _compiler_context, UEdGraph* _source_graph)
{
	Super::ExpandNode(_compiler_context, _source_graph);

	const auto schema = _compiler_context.GetSchema();
	if (IsInvalid(schema))
	{
		BreakAllNodeLinks();
		return;
	}

	UEdGraphPin* type_pin = FindPin(GetTypePinName());
	UEdGraphPin* sub_type_pin = FindPin(GetSubTypePinName());
	UEdGraphPin* serial_pin = FindPin(GetSerialPinName());
	UEdGraphPin* return_pin = FindPin(GetReturnValuePinName());

	if (IsAnyInvalid(type_pin, sub_type_pin, serial_pin, return_pin))
	{
		_compiler_context.MessageLog.Error(*LOCTEXT("MakeItemID_MissingPins", "Make ItemID: required pin is missing. @@").ToString(), this);

		BreakAllNodeLinks();
		return;
	}

	UFunction* make_item_id_function = UItemIDHelper::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UItemIDHelper, MakeItemID));
	if (IsInvalid(make_item_id_function))
	{
		_compiler_context.MessageLog.Error(*LOCTEXT("MakeItemID_FunctionMissing", "Make ItemID: MakeItemID function not found. @@").ToString(), this);

		BreakAllNodeLinks();
		return;
	}

	auto call_function_node = _compiler_context.SpawnIntermediateNode<UK2Node_CallFunction>(this, _source_graph);
	call_function_node->SetFromFunction(make_item_id_function);
	call_function_node->AllocateDefaultPins();

	UEdGraphPin* func_type_pin = call_function_node->FindPinChecked(TEXT("_type"));
	UEdGraphPin* func_sub_type_pin = call_function_node->FindPinChecked(TEXT("_sub_type"));
	UEdGraphPin* func_serial_pin = call_function_node->FindPinChecked(TEXT("_serial"));
	UEdGraphPin* func_return_pin = call_function_node->GetReturnValuePin();

	// Type
	if (!type_pin->LinkedTo.IsEmpty())
	{
		_compiler_context.MovePinLinksToIntermediate(*type_pin, *func_type_pin);
	}
	else
	{
		func_type_pin->DefaultValue = type_pin->DefaultValue;
	}

	// SubType
	if (!sub_type_pin->LinkedTo.IsEmpty())
	{
		_compiler_context.MovePinLinksToIntermediate(*sub_type_pin, *func_sub_type_pin);
	}
	else
	{
		FString sub_type_default_value = TEXT("0");

		if (const UEnum* sub_type_enum = Cast<UEnum>(sub_type_pin->PinType.PinSubCategoryObject.Get()))
		{
			const int64 enum_value = sub_type_enum->GetValueByNameString(sub_type_pin->DefaultValue);
			if (enum_value != INDEX_NONE)
			{
				sub_type_default_value = FString::FromInt(static_cast<int32>(enum_value));
			}
		}
		else
		{
			sub_type_default_value = sub_type_pin->DefaultValue;
		}

		func_sub_type_pin->DefaultValue = sub_type_default_value;
	}

	// Serial
	if (!serial_pin->LinkedTo.IsEmpty())
	{
		_compiler_context.MovePinLinksToIntermediate(*serial_pin, *func_serial_pin);
	}
	else
	{
		func_serial_pin->DefaultValue = serial_pin->DefaultValue;
	}

	// ReturnValue
	_compiler_context.MovePinLinksToIntermediate(*return_pin, *func_return_pin);

	BreakAllNodeLinks();
}

void UK2Node_MakeItemID::PinDefaultValueChanged(UEdGraphPin* _pin)
{
	Super::PinDefaultValueChanged(_pin);

	if (IsInvalid(_pin))
		return;

	if (_pin->PinName == GetTypePinName())
	{
		_CachedType = GetItemTypeFromPinDefault(_pin);
		RefreshSubTypePin();
	}
}

void UK2Node_MakeItemID::PostReconstructNode()
{
	Super::PostReconstructNode();

	UEdGraphPin* type_pin = FindPin(GetTypePinName());
	if (IsValid(type_pin))
	{
		type_pin->DefaultValue = TEnumToString(_CachedType);
	}

	UEdGraphPin* sub_type_pin = FindPin(GetSubTypePinName());
	if (IsInvalid(sub_type_pin))
		return;

	const UEnum* sub_type_enum = FItemID::GetSubTypeEnum(_CachedType);
	if (IsInvalid(sub_type_enum))
	{
		sub_type_pin->DefaultValue = TEXT("0");
		return;
	}

	// 기본값이 현재 enum에서 유효하지 않으면 첫 유효값으로 교체
	const int64 current_value = sub_type_enum->GetValueByNameString(sub_type_pin->DefaultValue);
	if (current_value != INDEX_NONE && IsValidEnumValue(sub_type_enum, current_value))
	{
		return;
	}

	const int32 num = sub_type_enum->NumEnums();
	for (int32 i = 0; i < num; ++i)
	{
		if (sub_type_enum->HasMetaData(TEXT("Hidden"), i))
		{
			continue;
		}

		const int64 value = sub_type_enum->GetValueByIndex(i);
		if (!IsValidEnumValue(sub_type_enum, value))
		{
			continue;
		}

		sub_type_pin->DefaultValue = sub_type_enum->GetNameStringByValue(value);
		return;
	}

	sub_type_pin->DefaultValue = TEXT("0");
}

void UK2Node_MakeItemID::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& _old_pins)
{
	// old type pin default를 먼저 읽어서 cached type 결정
	for (auto old_pin : _old_pins)
	{
		if (IsInvalid(old_pin))
			continue;

		if (old_pin->PinName == GetTypePinName())
		{
			_CachedType = GetItemTypeFromPinDefault(old_pin);
			break;
		}
	}

	Super::ReallocatePinsDuringReconstruction(_old_pins);
}

FText UK2Node_MakeItemID::GetNodeTitle(ENodeTitleType::Type _title_type) const
{
	return LOCTEXT("NodeTitle", "Make ItemID");
}

FText UK2Node_MakeItemID::GetTooltipText() const
{
	return LOCTEXT("Tooltip", "Creates an ItemID from Type, SubType, and Serial.");
}

FText UK2Node_MakeItemID::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "Item");
}

void UK2Node_MakeItemID::GetMenuActions(FBlueprintActionDatabaseRegistrar& _action_registrar) const
{
	UClass* node_class = GetClass();

	if (_action_registrar.IsOpenForRegistration(node_class))
	{
		UBlueprintNodeSpawner* spawner = UBlueprintNodeSpawner::Create(node_class);
		_action_registrar.AddBlueprintAction(node_class, spawner);
	}
}

EItemType UK2Node_MakeItemID::GetCurrentItemType() const
{
	UEdGraphPin* type_pin = FindPin(GetTypePinName());
	if (IsInvalid(type_pin))
	{
		return EItemType::NA;
	}

	const UEnum* item_type_enum = StaticEnum<EItemType>();
	if (IsInvalid(item_type_enum))
	{
		return EItemType::NA;
	}

	const int64 value = item_type_enum->GetValueByNameString(type_pin->DefaultValue);
	if (value == INDEX_NONE)
	{
		return EItemType::NA;
	}

	return static_cast<EItemType>(value);
}

EItemType UK2Node_MakeItemID::GetItemTypeFromPinDefault(const UEdGraphPin* _type_pin) const
{
	if (IsInvalid(_type_pin))
		return EItemType::NA;

	const UEnum* item_type_enum = StaticEnum<EItemType>();
	if (IsInvalid(item_type_enum))
		return EItemType::NA;

	const int64 value = item_type_enum->GetValueByNameString(_type_pin->DefaultValue);
	if (value == INDEX_NONE)
	{
		return EItemType::NA;
	}

	return static_cast<EItemType>(value);
}

void UK2Node_MakeItemID::RefreshSubTypePin()
{
	ReconstructNode();
}

#undef LOCTEXT_NAMESPACE