// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneGraphNode_Transition.h"

namespace
{
	static const FName StorySceneTransitionPinCategory(TEXT("StoryFlow"));
}

void UStorySceneGraphNode_Transition::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, StorySceneTransitionPinCategory, NAME_None, TEXT("In"));
}

FText UStorySceneGraphNode_Transition::GetNodeTitle(ENodeTitleType::Type _title_type) const
{
	if (_NextSceneID.IsValid())
	{
		return FText::Format(FText::FromString(TEXT("Transition\n{0}")), FText::FromName(_NextSceneID.Get()));
	}

	return FText::FromString(TEXT("Transition"));
}

FText UStorySceneGraphNode_Transition::GetTooltipText() const
{
	return _NextSceneID.IsValid()
		? FText::Format(FText::FromString(TEXT("Next Scene: {0}")), FText::FromName(_NextSceneID.Get()))
		: FText::FromString(TEXT("Next Scene is not set."));
}

void UStorySceneGraphNode_Transition::ClearCompileMessage()
{
	bHasCompilerMessage = false;
	ErrorType = 0;
	ErrorMsg.Empty();
}

void UStorySceneGraphNode_Transition::SetCompileError(const FString& _error_message)
{
	bHasCompilerMessage = true;
	ErrorType = 1;
	ErrorMsg = _error_message;
}
