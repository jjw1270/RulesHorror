// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "SGraphNodeDefault.h"

class UStorySceneGraphNode_Shot;

class SStorySceneGraphNode_Shot : public SGraphNodeDefault
{
public:
	SLATE_BEGIN_ARGS(SStorySceneGraphNode_Shot)
		: _GraphNodeObj(nullptr)
	{
	}

	SLATE_ARGUMENT(UStorySceneGraphNode_Shot*, GraphNodeObj)
	SLATE_END_ARGS()

	void Construct(const FArguments& _args);
	virtual void UpdateGraphNode() override;

protected:
	TSharedRef<SWidget> CreatePlayButtonWidget();

private:
	FReply OnClickPlayButton();
	FText GetPlayButtonTooltipText() const;
	EVisibility GetPlayButtonVisibility() const;

	UStorySceneGraphNode_Shot* GetStorySceneGraphNode() const;
};
