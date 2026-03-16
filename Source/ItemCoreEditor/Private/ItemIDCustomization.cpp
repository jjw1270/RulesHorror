// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemIDCustomization.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"

TSharedRef<IPropertyTypeCustomization> FItemIDCustomization::MakeInstance()
{
	return MakeShared<FItemIDCustomization>();
}

void FItemIDCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> _property_handle, FDetailWidgetRow& _header_row, IPropertyTypeCustomizationUtils& _customization_utils)
{
	_ValuePropertyHandle = _property_handle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FItemID, Value));
	
	GenerateTypeOptions();
	GenerateSubTypeOptions();

	_header_row
		.NameContent()
		[
			_property_handle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					[
						SNew(SHorizontalBox)

						// type
						+ SHorizontalBox::Slot()
							.MinWidth(100.0f)
							.MaxWidth(140.0f)
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 12.0f, 0.0f)
							[
								SNew(SComboBox<TSharedPtr<EItemType>>)
									.ContentPadding(FVector2D(8.0f, 0.0f))
									.OptionsSource(&_TypeOptions)
									.OnGenerateWidget(this, &FItemIDCustomization::MakeTypeWidget)
									.OnSelectionChanged(this, &FItemIDCustomization::OnTypeChanged)
									.InitiallySelectedItem(_SelectedType)
									[
										SNew(STextBlock)
											.Text_Lambda([this]()
												{
													return GetSelectedTypeText();
												}
											)
											.ToolTipText_Lambda([this]()
												{
													return GetSelectedTypeText();
												}
											)
									]
							]

						// SubType
						+ SHorizontalBox::Slot()
							.MinWidth(100.0f)
							.MaxWidth(140.0f)
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 12.0f, 0.0f)
							[
								SAssignNew(_SubTypeComboBox, SComboBox<TSharedPtr<uint8>>)
									.ContentPadding(FVector2D(8.0f, 0.0f))
									.OptionsSource(&_SubTypeOptions)
									.OnGenerateWidget(this, &FItemIDCustomization::MakeSubTypeWidget)
									.OnSelectionChanged(this, &FItemIDCustomization::OnSubTypeChanged)
									.InitiallySelectedItem(_SelectedSubType)
									[
										SNew(STextBlock)
											.Text_Lambda([this]()
												{
													return GetSelectedSubTypeText();
												}
											)
											.ToolTipText_Lambda([this]()
												{
													return GetSelectedSubTypeText();
												}
											)
									]
							]

						// Serial
						+ SHorizontalBox::Slot()
							.MinWidth(60.0f)
							.MaxWidth(60.0f)
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 12.0f, 0.0f)
							[
								SNew(SNumericEntryBox<int32>)
									.MinValue(0)
									.MaxValue(9999)
									.AllowSpin(false)
									.Value(this, &FItemIDCustomization::GetSerialValue)
									.OnValueChanged(this, &FItemIDCustomization::OnSerialChanged)
									.OnValueCommitted(this, &FItemIDCustomization::OnSerialCommitted)
							]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					[
						SNew(SHorizontalBox)

						// Valid Check
						+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(STextBlock)
									.Text(FText::FromString(TEXT("●")))
									.ColorAndOpacity_Lambda([this]()
										{
											return GetItemID().Validate().IsValid()
												? FSlateColor(FLinearColor(0.1f, 0.8f, 0.2f, 1.0f))
												: FSlateColor(FLinearColor(0.9f, 0.1f, 0.1f, 1.0f));
										}
									)
									.ToolTipText_Lambda([this]()
										{
											return FText::FromString(GetItemID().Validate().Reason);
										}
									)
							]

						// Display Number
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text_Lambda([this]()
										{
											return FText::FromString(GetItemID().ToString());
										}
									)
							]
					]
		];
}

FItemID FItemIDCustomization::GetItemID() const
{
	FItemID item_id;

	if (IsValid(_ValuePropertyHandle))
	{
		uint32 value = 0;
		if (_ValuePropertyHandle->GetValue(value) == FPropertyAccess::Success)
		{
			item_id = value;
		}
	}
	
	return item_id;
}

void FItemIDCustomization::SetItemID(const FItemID& _item_id)
{
	if (IsValid(_ValuePropertyHandle))
	{
		const FPropertyAccess::Result result = _ValuePropertyHandle->SetValue(static_cast<uint32>(_item_id));

		if (result != FPropertyAccess::Success)
		{
			TRACE_ERROR(TEXT("SetValue Failure"));
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Type
void FItemIDCustomization::GenerateTypeOptions()
{
	if (_TypeOptions.IsEmpty() == false)
		return;

	const EItemType current_type = GetItemID().GetType();

	for (EItemType item_type : TEnumRange<EItemType>())
	{
		if (IsValidEnumValue(item_type) == false)
			continue;

		auto type_option = MakeShared<EItemType>(item_type);

		_TypeOptions.Add(type_option);

		if (item_type == current_type)
		{
			_SelectedType = type_option;
		}
	}
}

TSharedRef<SWidget> FItemIDCustomization::MakeTypeWidget(TSharedPtr<EItemType> _item)
{
	return SNew(STextBlock)
		.Text(FText::FromString(_item.IsValid() ? TEnumToString(*_item) : TEXT("Invalid")));
}

void FItemIDCustomization::OnTypeChanged(TSharedPtr<EItemType> _item, ESelectInfo::Type _select_info)
{
	if (IsInvalid(_item))
		return;

	_SelectedType = _item;

	FItemID item_id = GetItemID();
	item_id.SetType(*_SelectedType);
	SetItemID(item_id);

	GenerateSubTypeOptions();

	if (IsValid(_SubTypeComboBox))
	{
		_SubTypeComboBox->RefreshOptions();
		_SubTypeComboBox->SetSelectedItem(_SelectedSubType);
	}
}

FText FItemIDCustomization::GetSelectedTypeText() const
{
	if (IsValid(_SelectedType))
	{
		return FText::FromString(TEnumToString(*_SelectedType));
	}

	return FText::FromString(TEXT("Invalid"));
}
#pragma endregion Type
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region SubType
void FItemIDCustomization::GenerateSubTypeOptions()
{
	_SubTypeOptions.Reset();
	_SelectedSubType.Reset();

	const FItemID item_id = GetItemID();
	const UEnum* sub_type_enum = item_id.GetSubTypeEnum(item_id.GetType());
	if (IsInvalid(sub_type_enum))
		return;

	const uint8 current_sub_type = item_id.GetSubType();

	// ENUM_MAX 가 포함된 값이라 -1을 합니다.
	const int32 num = sub_type_enum->NumEnums() - 1;

	for (int32 i = 0; i < num; ++i)
	{
		if (sub_type_enum->HasMetaData(TEXT("Hidden"), i))
			continue;

		const int64 sub_type = sub_type_enum->GetValueByIndex(i);
		if (sub_type < 0 || sub_type > MAX_uint8)
			continue;

		auto sub_type_option = MakeShared<uint8>(static_cast<uint8>(sub_type));

		_SubTypeOptions.Add(sub_type_option);

		if (sub_type == current_sub_type)
		{
			_SelectedSubType = sub_type_option;
		}
	}
}

TSharedRef<SWidget> FItemIDCustomization::MakeSubTypeWidget(TSharedPtr<uint8> _item)
{
	const FItemID item_id = GetItemID();
	const UEnum* sub_type_enum = item_id.GetSubTypeEnum(item_id.GetType());

	FString text = TEXT("Invalid");

	if (_item.IsValid() && sub_type_enum != nullptr)
	{
		text = sub_type_enum->GetNameStringByValue(*_item);
	}

	return SNew(STextBlock)
		.Text(FText::FromString(text));
}

void FItemIDCustomization::OnSubTypeChanged(TSharedPtr<uint8> _item, ESelectInfo::Type _select_info)
{
	if (IsInvalid(_item))
		return;
	_SelectedSubType = _item;

	FItemID item_id = GetItemID();
	item_id.SetSubType(*_SelectedSubType);

	SetItemID(item_id);
}

FText FItemIDCustomization::GetSelectedSubTypeText() const
{
	if (IsValid(_SelectedSubType))
	{
		const FItemID item_id = GetItemID();
		const UEnum* sub_type_enum = item_id.GetSubTypeEnum(item_id.GetType());

		if (IsValid(sub_type_enum))
		{
			return FText::FromString(sub_type_enum->GetNameStringByValue(*_SelectedSubType));
		}
	}

	return FText::FromString(TEXT("Invalid"));
}
#pragma endregion SubType
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region Serial
TOptional<int32> FItemIDCustomization::GetSerialValue() const
{
	return static_cast<int32>(GetItemID().GetSerial());
}

void FItemIDCustomization::OnSerialChanged(int32 _value)
{
	_value = FMath::Clamp(_value, 0, 9999);

	FItemID item_id = GetItemID();
	item_id.SetSerial(static_cast<uint16>(_value));

	SetItemID(item_id);
}

void FItemIDCustomization::OnSerialCommitted(int32 _value, ETextCommit::Type _commit_type)
{
	_value = FMath::Clamp(_value, 0, 9999);

	FItemID item_id = GetItemID();
	item_id.SetSerial(static_cast<uint16>(_value));

	SetItemID(item_id);
}
#pragma endregion Serial
