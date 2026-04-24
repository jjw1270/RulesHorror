// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "StorySceneGraphNode_Entry.generated.h"

UCLASS()
class UStorySceneGraphNode_Entry : public UEdGraphNode
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type _title_type) const override { return FText::FromString(TEXT("Entry")); }
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }

	void ClearCompileMessage();
	void SetCompileError(const FString& _error_message);
};
