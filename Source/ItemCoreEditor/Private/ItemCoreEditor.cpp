// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemCoreEditor.h"
#include "PropertyEditorModule.h"
#include "ItemIDCustomization.h"

IMPLEMENT_MODULE(FItemCoreEditorModule, ItemCoreEditor)

void FItemCoreEditorModule::StartupModule()
{
	FPropertyEditorModule& property_module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	property_module.RegisterCustomPropertyTypeLayout("ItemID", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemIDCustomization::MakeInstance));

	property_module.NotifyCustomizationModuleChanged();
}

void FItemCoreEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& property_module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		property_module.UnregisterCustomPropertyTypeLayout("ItemID");
	}
}
