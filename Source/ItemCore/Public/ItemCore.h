// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

static const FName ItemRegistryLog(TEXT("ItemRegistry"));

class FItemCoreModule : public IModuleInterface
{
protected:
#if !UE_BUILD_SHIPPING
	const FString ConsoleCommand_ToggleShowItemID = TEXT("ToggleShowItemID");
#endif

public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
