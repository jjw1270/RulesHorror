// Copyright (c) 2026 장윤제. All rights reserved.

#include "ItemCore.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
#include "ItemTableRow.h"
#endif

#define LOCTEXT_NAMESPACE "FItemCoreModule"

void FItemCoreModule::StartupModule()
{
#if !UE_BUILD_SHIPPING
	FConsoleCommandDelegate delegate;
	delegate.BindStatic(&FItemTableRow::ToggleShowItemIDOnDisplayName);

	IConsoleManager::Get().RegisterConsoleCommand(*ConsoleCommand_ToggleShowItemID, TEXT("Toggle Show ItemID"), delegate);
#endif
}

void FItemCoreModule::ShutdownModule()
{
#if !UE_BUILD_SHIPPING
	IConsoleManager::Get().UnregisterConsoleObject(*ConsoleCommand_ToggleShowItemID);
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FItemCoreModule, ItemCore)