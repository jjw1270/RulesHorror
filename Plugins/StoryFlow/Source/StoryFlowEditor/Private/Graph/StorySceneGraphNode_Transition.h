// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "StoryFlowDefines.h"
#include "StorySceneGraphNode_Transition.generated.h"

UCLASS()
class UStorySceneGraphNode_Transition : public UEdGraphNode
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "StoryFlow")
	FStorySceneID _NextSceneID;

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type _title_type) const override;
	virtual FText GetTooltipText() const override;
	virtual bool CanDuplicateNode() const override { return false; }

	void ClearCompileMessage();
	void SetCompileError(const FString& _error_message);

	const FStorySceneID& GetNextSceneID() const { return _NextSceneID; }
};
