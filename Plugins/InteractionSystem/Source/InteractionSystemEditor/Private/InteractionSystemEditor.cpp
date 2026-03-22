// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractionSystemEditor.h"
#include "CommonUtils.h"
#include "Editor.h"
#include "UnrealEd.h"
#include "InteractionComponentVisualizer.h"
#include "InteractionComponent.h"

#define LOCTEXT_NAMESPACE "FInteractionSystemEditor"

void FInteractionSystemEditor::StartupModule()
{
	if (IsInvalid(GUnrealEd))
		return;

	TSharedPtr<FComponentVisualizer> visualizer = MakeShared<FInteractionComponentVisualizer>();
	GUnrealEd->RegisterComponentVisualizer(UInteractionComponent::StaticClass()->GetFName(), visualizer);

	visualizer->OnRegister();
}

void FInteractionSystemEditor::ShutdownModule()
{
	if (IsInvalid(GUnrealEd))
		return;

	GUnrealEd->UnregisterComponentVisualizer(UInteractionComponent::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInteractionSystemEditor, InteractionSystemEditor);