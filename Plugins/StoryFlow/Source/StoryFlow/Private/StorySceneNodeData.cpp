// Copyright (c) 2026 장윤제. All rights reserved.

#include "StorySceneNodeData.h"
#include "StoryShotBase.h"
#include "CommonUtils.h"

#if WITH_EDITOR
namespace
{
	static FText MakeDisplayNameFromTemplate(const UObject* _template)
	{
		if (IsInvalid(_template))
		{
			return FText::GetEmpty();
		}

		FString template_name;
		const UClass* template_class = _template->GetClass();
		if (IsValid(template_class) && IsValid(template_class->ClassGeneratedBy))
		{
			template_name = template_class->ClassGeneratedBy->GetName();
		}
		else
		{
			template_name = _template->GetName();
		}

		return FText::FromString(FName::NameToDisplayString(template_name, false));
	}
}

void UStorySceneNodeData::PostEditChangeProperty(FPropertyChangedEvent& _property_changed_event)
{
	Super::PostEditChangeProperty(_property_changed_event);

	if (_property_changed_event.GetPropertyName() != GET_MEMBER_NAME_CHECKED(UStorySceneNodeData, _ShotTemplate))
	{
		return;
	}

	if (_DisplayName.IsEmpty() == false || IsInvalid(_ShotTemplate))
	{
		return;
	}

	Modify();
	_DisplayName = MakeDisplayNameFromTemplate(_ShotTemplate);
	MarkPackageDirty();
}

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
