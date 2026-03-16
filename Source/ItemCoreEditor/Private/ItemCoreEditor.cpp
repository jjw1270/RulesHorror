// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemCoreEditor.h"
#include "PropertyEditorModule.h"
#include "ItemIDCustomization.h"
#include "Engine/Engine.h"
#include "MessageLogModule.h"
#include "ToolMenus.h"
#include "ItemRegistrySubsystem.h"
#include "ItemCoreEditorStyle.h"

#define LOCTEXT_NAMESPACE "FItemCoreEditorModule"

void FItemCoreEditorModule::StartupModule()
{
	// editor style
	FItemCoreEditorStyle::Initialize();

	// message log
	FMessageLogModule& message_log_module = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	message_log_module.RegisterLogListing(TEXT("ItemRegistry"), FText::FromString(TEXT("Item Registry")));

	// property customization
	FPropertyEditorModule& property_module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	property_module.RegisterCustomPropertyTypeLayout("ItemID", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemIDCustomization::MakeInstance));

	property_module.NotifyCustomizationModuleChanged();

	// tool menu
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FItemCoreEditorModule::RegisterMenus));
	}
}

void FItemCoreEditorModule::ShutdownModule()
{
	// editor style
	FItemCoreEditorStyle::Shutdown();

	// property customization
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& property_module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		property_module.UnregisterCustomPropertyTypeLayout("ItemID");
	}

	// tool menu
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FItemCoreEditorModule::RegisterMenus()
{
	if (_IsMenuRegistered)
		return;

	AddItemRegistryMenuEntry();
	_IsMenuRegistered = true;
}

void FItemCoreEditorModule::AddItemRegistryMenuEntry()
{
	UToolMenu* toolbar_menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User"));
	if (IsInvalid(toolbar_menu))
		return;

	FToolMenuSection& section = toolbar_menu->FindOrAddSection(TEXT("RefreshItemRegistry"));

	section.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("RefreshItemRegistry"),
		FUIAction(FExecuteAction::CreateRaw(this, &FItemCoreEditorModule::OnClicked_RefreshRegistry)),
		LOCTEXT("RefreshItemRegistry_Label", "Refresh Registry"),
		LOCTEXT("RefreshItemRegistry_ToolTip", "Refresh item registry."),
		FSlateIcon(FItemCoreEditorStyle::GetStyleSetName(), "ItemRegistry.Refresh")
	));
}

void FItemCoreEditorModule::OnClicked_RefreshRegistry()
{
	if (IsInvalid(GEngine))
		return;

	auto item_registry = GEngine->GetEngineSubsystem<UItemRegistrySubsystem>();
	if (IsValid(item_registry))
	{
		item_registry->RefreshRegistry();
	}
}
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FItemCoreEditorModule, ItemCoreEditor)
