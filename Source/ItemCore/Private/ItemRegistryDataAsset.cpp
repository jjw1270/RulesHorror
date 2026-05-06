// Copyright (c) 2026 장윤제. All rights reserved.

#include "ItemRegistryDataAsset.h"


FPrimaryAssetId UItemRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ItemRegistry"), GetFName());
}
