// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneGraphNode_Entry.h"

static const FName StoryFlowPinCategory(TEXT("StoryFlow"));

void UStorySceneGraphNode_Entry::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, StoryFlowPinCategory, NAME_None, TEXT("Next"));
}

void UStorySceneGraphNode_Entry::ClearCompileMessage()
{
	bHasCompilerMessage = false;
	ErrorType = 0;
	ErrorMsg.Empty();
}

void UStorySceneGraphNode_Entry::SetCompileError(const FString& _error_message)
{
	bHasCompilerMessage = true;
	ErrorType = 1;
	ErrorMsg = _error_message;
}
