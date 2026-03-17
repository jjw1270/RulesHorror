// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemRegistryValidationSubsystem.h"
#include "ItemRegistrySubsystem.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Misc/MessageDialog.h"
#endif

void UItemRegistryValidationSubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

	if (IsInvalid(GEngine))
		return;

	auto item_registry = GEngine->GetEngineSubsystem<UItemRegistrySubsystem>();
	if (IsInvalid(item_registry))
		return;

	const bool is_success = item_registry->RefreshRegistry();

#if WITH_EDITOR
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Refresh Item Registry Failed.\n\n")));

	if (IsValid(GEditor))
	{
		GEditor->RequestEndPlayMap();
	}
#else
	checkf(is_success, TEXT("Refresh Item Registry Failed."));
#endif
}

void UItemRegistryValidationSubsystem::Deinitialize()
{
	Super::Deinitialize();
}
