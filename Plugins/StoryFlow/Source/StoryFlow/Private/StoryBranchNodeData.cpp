// Copyright (c) 2026 장윤제. All rights reserved.

#include "StoryBranchNodeData.h"
#include "StorySceneNodeData.h"

#if WITH_EDITOR
void UStoryBranchNodeData::SetBranchID(const FStoryBranchID& _branch_id)
{
	if (_BranchID == _branch_id)
	{
		return;
	}

	Modify();
	_BranchID = _branch_id;
	MarkPackageDirty();
}

void UStoryBranchNodeData::SetBranchCount(int32 _branch_count)
{
	const int32 branch_count = FMath::Max(_branch_count, 1);
	if (_BranchCount == branch_count)
	{
		return;
	}

	Modify();
	_BranchCount = branch_count;
	MarkPackageDirty();
}

void UStoryBranchNodeData::SetNextLinksByPinIndex(const TMap<int32, FStorySceneBranchLink>& _next_links_by_pin_index)
{
	if (_NextLinksByPinIndex.OrderIndependentCompareEqual(_next_links_by_pin_index))
	{
		return;
	}

	Modify();
	_NextLinksByPinIndex = _next_links_by_pin_index;
	MarkPackageDirty();
}
#endif

FText UStoryBranchNodeData::GetDisplayNameText() const
{
	if (_DisplayName.IsEmpty() == false)
	{
		return _DisplayName;
	}

	if (_BranchID.IsValid())
	{
		return FText::FromName(_BranchID.Get());
	}

	return FText::FromString(TEXT("Branch"));
}
