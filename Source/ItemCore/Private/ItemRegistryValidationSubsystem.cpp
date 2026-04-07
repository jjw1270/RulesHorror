// Copyright (c) 2026 장윤제. All rights reserved.

#include "ItemRegistryValidationSubsystem.h"
#include "ItemRegistrySubsystem.h"
#include "CommonUtils.h"

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
	if (is_success == false)
	{
		EDITOR_POPUP(TEXT("Refresh Item Registry 실패!"));
	}
#else
	checkf(is_success, TEXT("Refresh Item Registry Failed."));
#endif
}

void UItemRegistryValidationSubsystem::Deinitialize()
{
	Super::Deinitialize();
}
