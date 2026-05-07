// Copyright (c) 2026 장윤제. All rights reserved.

#include "ItemTableReferenceCustomization.h"
#include "AssetRegistry/AssetData.h"
#include "DetailWidgetRow.h"
#include "Engine/DataTable.h"
#include "ItemDefines.h"
#include "ItemTableRow.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

TSharedRef<IPropertyTypeCustomization> FItemTableReferenceCustomization::MakeInstance()
{
	return MakeShared<FItemTableReferenceCustomization>();
}

void FItemTableReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> _property_handle, FDetailWidgetRow& _header_row, IPropertyTypeCustomizationUtils& _customization_utils)
{
	const TSharedPtr<IPropertyHandle> data_table_property_handle = _property_handle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FItemTableReference, DataTable));

	_header_row
		.NameContent()
		[
			_property_handle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(0.0f)
		[
			SNew(SObjectPropertyEntryBox)
				.PropertyHandle(data_table_property_handle)
				.AllowedClass(UDataTable::StaticClass())
				.OnShouldFilterAsset_Static(&FItemTableReferenceCustomization::ShouldFilterAsset)
		];
}

bool FItemTableReferenceCustomization::ShouldFilterAsset(const FAssetData& _asset_data)
{
	static const FName RowStructureTagName(TEXT("RowStructure"));

	FString row_structure;
	if (_asset_data.GetTagValue<FString>(RowStructureTagName, row_structure) == false)
		return true;

	const UScriptStruct* row_struct = UClass::TryFindTypeSlow<UScriptStruct>(row_structure);
	if (row_struct == nullptr)
		return true;

	return row_struct->IsChildOf(FItemTableRow::StaticStruct()) == false;
}
