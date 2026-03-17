// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "ItemID.h"

class SWidget;

class FItemIDCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> _property_handle, FDetailWidgetRow& _header_row, IPropertyTypeCustomizationUtils& _customization_utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> _property_handle, IDetailChildrenBuilder& _child_builder, IPropertyTypeCustomizationUtils& _customization_utils) override {}

protected:
	TSharedPtr<IPropertyHandle> _ValuePropertyHandle;

protected:
	FItemID GetItemID() const;
	void SetItemID(const FItemID& _item_id);

#pragma region Type
protected:
	TArray<TSharedPtr<EItemType>> _TypeOptions;
	TSharedPtr<EItemType> _SelectedType;

protected:
	void GenerateTypeOptions();

	TSharedRef<SWidget> MakeTypeWidget(TSharedPtr<EItemType> _item);
	void OnTypeChanged(TSharedPtr<EItemType> _item, ESelectInfo::Type _select_info);

	FText GetSelectedTypeText() const;
#pragma endregion Type
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region SubType
protected:
	TSharedPtr<class SComboBox<TSharedPtr<uint8>>> _SubTypeComboBox;

	TArray<TSharedPtr<uint8>> _SubTypeOptions;
	TSharedPtr<uint8> _SelectedSubType;

protected:
	void GenerateSubTypeOptions();

	TSharedRef<SWidget> MakeSubTypeWidget(TSharedPtr<uint8> _item);
	void OnSubTypeChanged(TSharedPtr<uint8> _item, ESelectInfo::Type _select_info);

	FText GetSelectedSubTypeText() const;
#pragma endregion SubType
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Serial
protected:
	TOptional<int32> GetSerialValue() const;
	void OnSerialCommitted(int32 _value, ETextCommit::Type _commit_type);
#pragma endregion Serial
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Picker
protected:
	struct FItemIDPickerOption
	{
		FItemID ItemID;
		FText DisplayText;
	};

protected:
	TSharedPtr<class SComboButton> _ItemIDPickerButton;
	TArray<TSharedPtr<FItemIDPickerOption>> _ItemIDPickerOptions;

protected:
	void GenerateItemIDPickerOptions();

	TSharedRef<SWidget> GetItemIDPickerMenu();
	void OnItemIDPicked(TSharedPtr<FItemIDPickerOption> _item);

	FText GetPickerButtonText() const;
#pragma endregion Picker
};
