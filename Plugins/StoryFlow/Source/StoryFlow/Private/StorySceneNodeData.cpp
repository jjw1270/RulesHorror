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

void UStorySceneNodeData::SetNextLinks(const TArray<FStorySceneBranchLink>& _next_links)
{
	if (_NextLinks == _next_links)
	{
		return;
	}

	Modify();
	_NextLinks = _next_links;
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
