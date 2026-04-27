// Copyright (c) 2026 장윤제. All rights reserved.

#include "StorySceneNodeData.h"

#if WITH_EDITOR
void UStorySceneNodeData::SetShotID(const FStoryShotID& _shot_id)
{
	if (_ShotID == _shot_id)
	{
		return;
	}

	Modify();
	_ShotID = _shot_id;
	MarkPackageDirty();
}

void UStorySceneNodeData::SetNextLink(const FStorySceneBranchLink& _next_link)
{
	if (_NextLink == _next_link)
	{
		return;
	}

	Modify();
	_NextLink = _next_link;
	MarkPackageDirty();
}
#endif

FText UStorySceneNodeData::GetDisplayNameText() const
{
	if (_DisplayName.IsEmpty() == false)
	{
		return _DisplayName;
	}

	if (_ShotID.IsValid())
	{
		return FText::FromName(_ShotID.Get());
	}

	return FText::FromString(TEXT("Shot"));
}
