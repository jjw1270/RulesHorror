// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneGraphNode_Entry.h"

namespace
{
	static const FName StorySceneEntryPinCategory(TEXT("StoryFlow"));
}

void UStorySceneGraphNode_Entry::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, StorySceneEntryPinCategory, NAME_None, TEXT("Next"));
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
