// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

struct FAssetData;

class FItemTableReferenceCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> _property_handle, FDetailWidgetRow& _header_row, IPropertyTypeCustomizationUtils& _customization_utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> _property_handle, IDetailChildrenBuilder& _child_builder, IPropertyTypeCustomizationUtils& _customization_utils) override {}

private:
	static bool ShouldFilterAsset(const FAssetData& _asset_data);
};
