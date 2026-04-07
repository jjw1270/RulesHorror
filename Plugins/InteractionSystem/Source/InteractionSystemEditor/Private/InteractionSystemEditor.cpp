// Copyright (c) 2026 장윤제. All rights reserved.

#include "InteractionSystemEditor.h"
#include "CommonUtils.h"
#include "Editor.h"
#include "UnrealEd.h"
#include "InteractorComponentVisualizer.h"
#include "InteractorComponent.h"

#define LOCTEXT_NAMESPACE "FInteractionSystemEditor"

void FInteractionSystemEditor::StartupModule()
{
	if (IsInvalid(GUnrealEd))
		return;

	TSharedPtr<FComponentVisualizer> visualizer = MakeShared<FInteractorComponentVisualizer>();
	GUnrealEd->RegisterComponentVisualizer(UInteractorComponent::StaticClass()->GetFName(), visualizer);

	visualizer->OnRegister();
}

void FInteractionSystemEditor::ShutdownModule()
{
	if (IsInvalid(GUnrealEd))
		return;

	GUnrealEd->UnregisterComponentVisualizer(UInteractorComponent::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInteractionSystemEditor, InteractionSystemEditor);