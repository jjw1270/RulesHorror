#pragma once

#include "Modules/ModuleManager.h"

class FRuleHorroEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
