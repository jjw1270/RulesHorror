// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FRulesPluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
