// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FItemCoreEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool _IsMenuRegistered = false;

	void RegisterMenus();
	void AddItemRegistryMenuEntry();

	void OnClicked_RefreshRegistry();
};
