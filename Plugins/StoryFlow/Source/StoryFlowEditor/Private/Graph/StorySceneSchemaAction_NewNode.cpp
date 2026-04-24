// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneSchemaAction_NewNode.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

UEdGraphNode* FStorySceneSchemaAction_NewNode::PerformAction(UEdGraph* _parent_graph, UEdGraphPin* _from_pin, const FVector2D _location, bool _is_select_new_node)
{
	if (_parent_graph == nullptr || _NodeClass == nullptr)
	{
		return nullptr;
	}

	UEdGraphNode* new_node = NewObject<UEdGraphNode>(_parent_graph, _NodeClass, NAME_None, RF_Transactional);
	_parent_graph->Modify();
	new_node->CreateNewGuid();
	new_node->NodePosX = _location.X;
	new_node->NodePosY = _location.Y;
	_parent_graph->AddNode(new_node, true, _is_select_new_node);
	new_node->PostPlacedNewNode();
	new_node->AllocateDefaultPins();

	if (_from_pin)
	{
		new_node->AutowireNewNode(_from_pin);
	}

	return new_node;
}
