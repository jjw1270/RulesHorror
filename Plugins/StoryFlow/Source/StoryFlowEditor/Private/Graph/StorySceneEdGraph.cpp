// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneEdGraph.h"
#include "Graph/StorySceneGraphNode_Entry.h"
#include "Graph/StorySceneGraphNode_Shot.h"
#include "StorySceneAsset.h"
#include "StorySceneNodeData.h"
#include "CommonUtils.h"

void UStorySceneEdGraph::PostLoad()
{
	Super::PostLoad();
	EnsureEntryNode();
}

UStorySceneAsset* UStorySceneEdGraph::GetOwningSceneAsset() const
{
	return Cast<UStorySceneAsset>(GetOuter());
}

UStorySceneGraphNode_Entry* UStorySceneEdGraph::FindEntryNode() const
{
	for (UEdGraphNode* node : Nodes)
	{
		UStorySceneGraphNode_Entry* entry_node = Cast<UStorySceneGraphNode_Entry>(node);
		if (IsValid(entry_node))
		{
			return entry_node;
		}
	}

	return nullptr;
}

void UStorySceneEdGraph::EnsureEntryNode()
{
	if (FindEntryNode())
	{
		return;
	}

	UStorySceneGraphNode_Entry* entry_node = NewObject<UStorySceneGraphNode_Entry>(this, UStorySceneGraphNode_Entry::StaticClass(), NAME_None, RF_Transactional);
	entry_node->CreateNewGuid();
	entry_node->NodePosX = -400;
	entry_node->NodePosY = 0;
	AddNode(entry_node, true, false);
	entry_node->AllocateDefaultPins();
}

void UStorySceneEdGraph::RebuildRuntimeData()
{
	UStorySceneAsset* scene_asset = GetOwningSceneAsset();
	if (IsInvalid(scene_asset))
	{
		return;
	}

	FStoryShotID rebuilt_entry_shot_id;
	TMap<UStorySceneNodeData*, TArray<FStoryShotID>> rebuilt_next_shot_ids;

	for (UEdGraphNode* node : Nodes)
	{
		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsInvalid(shot_node) || IsInvalid(shot_node->GetShotNodeData()))
		{
			continue;
		}

		rebuilt_next_shot_ids.FindOrAdd(shot_node->GetShotNodeData());
	}

	if (UStorySceneGraphNode_Entry* entry_node = FindEntryNode())
	{
		if (UEdGraphPin* next_pin = entry_node->FindPin(TEXT("Next")))
		{
			if (next_pin->LinkedTo.Num() > 0)
			{
				UStorySceneGraphNode_Shot* target_node = Cast<UStorySceneGraphNode_Shot>(next_pin->LinkedTo[0]->GetOwningNode());
				if (IsValid(target_node) && IsValid(target_node->GetShotNodeData()))
				{
					rebuilt_entry_shot_id = target_node->GetShotNodeData()->GetShotID();
				}
			}
		}
	}

	for (UEdGraphNode* node : Nodes)
	{
		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsInvalid(shot_node) || IsInvalid(shot_node->GetShotNodeData()))
		{
			continue;
		}

		UEdGraphPin* next_pin = shot_node->FindPin(TEXT("Next"));
		if (next_pin == nullptr || next_pin->LinkedTo.Num() == 0)
		{
			continue;
		}

		UStorySceneGraphNode_Shot* target_node = Cast<UStorySceneGraphNode_Shot>(next_pin->LinkedTo[0]->GetOwningNode());
		if (IsInvalid(target_node) || IsInvalid(target_node->GetShotNodeData()))
		{
			continue;
		}

		TArray<FStoryShotID>& next_shot_ids = rebuilt_next_shot_ids.FindOrAdd(shot_node->GetShotNodeData());
		next_shot_ids.Reset();
		next_shot_ids.Add(target_node->GetShotNodeData()->GetShotID());
	}

	scene_asset->SetEntryShotID(rebuilt_entry_shot_id);

	for (const TPair<UStorySceneNodeData*, TArray<FStoryShotID>>& rebuilt_pair : rebuilt_next_shot_ids)
	{
		if (IsValid(rebuilt_pair.Key))
		{
			rebuilt_pair.Key->SetNextShotIDs(rebuilt_pair.Value);
		}
	}
}
