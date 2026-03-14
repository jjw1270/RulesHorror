#include "RulesHorrorEditor.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomization/ItemIDCustomization.h"

IMPLEMENT_MODULE(FRuleHorroEditorModule, RulesHorrorEditor)

void FRuleHorroEditorModule::StartupModule()
{
	FPropertyEditorModule& property_module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	property_module.RegisterCustomPropertyTypeLayout("ItemID", 	FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemIDCustomization::MakeInstance));

	property_module.NotifyCustomizationModuleChanged();
}

void FRuleHorroEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& property_module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		property_module.UnregisterCustomPropertyTypeLayout("ItemID");
	}
}