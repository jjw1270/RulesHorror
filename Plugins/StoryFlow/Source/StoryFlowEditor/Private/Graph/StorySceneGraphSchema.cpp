// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneGraphSchema.h"
#include "Graph/StorySceneSchemaAction_NewNode.h"
#include "Graph/StorySceneGraphNode_Entry.h"
#include "Graph/StorySceneGraphNode_Shot.h"
#include "StorySceneAsset.h"
#include "StorySceneNodeData.h"
#include "CommonUtils.h"

namespace
{
	static const FName StoryFlowPinCategory(TEXT("StoryFlow"));
}

void UStorySceneGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& _context_menu_builder) const
{
	TSharedPtr<FStorySceneSchemaAction_NewNode> action = MakeShared<FStorySceneSchemaAction_NewNode>(
		FText::FromString(TEXT("StoryFlow")),
		FText::FromString(TEXT("Add Shot")),
		FText::FromString(TEXT("Create a new shot node.")),
		0);

	action->_NodeClass = UStorySceneGraphNode_Shot::StaticClass();
	_context_menu_builder.AddAction(action);
}

const FPinConnectionResponse UStorySceneGraphSchema::CanCreateConnection(const UEdGraphPin* _a, const UEdGraphPin* _b) const
{
	if (_a == nullptr || _b == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pins."));
	}

	if (_a->Direction == _b->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Same direction."));
	}

	if (_a->PinType.PinCategory != StoryFlowPinCategory || _b->PinType.PinCategory != StoryFlowPinCategory)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Only StoryFlow pins are supported."));
	}

	const UEdGraphPin* output_pin = (_a->Direction == EGPD_Output) ? _a : _b;
	const UEdGraphPin* input_pin = (_a->Direction == EGPD_Input) ? _a : _b;

	if (output_pin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Output supports only one connection."));
	}

	if (input_pin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Input supports only one connection."));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool UStorySceneGraphSchema::TryCreateConnection(UEdGraphPin* _a, UEdGraphPin* _b) const
{
	return Super::TryCreateConnection(_a, _b);
}

void UStorySceneGraphSchema::BreakPinLinks(UEdGraphPin& _target_pin, bool _is_sends_node_notifcation) const
{
	Super::BreakPinLinks(_target_pin, _is_sends_node_notifcation);
}

void UStorySceneGraphSchema::BreakSinglePinLink(UEdGraphPin* _source_pin, UEdGraphPin* _target_pin) const
{
	Super::BreakSinglePinLink(_source_pin, _target_pin);
}
