// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

static const FName InteractionSystemLog(TEXT("InteractionSystem"));

class FInteractionSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
