// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneGraphNode_Shot.h"
#include "StorySceneAsset.h"
#include "StorySceneNodeData.h"
#include "StoryFlowDefines.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "CommonUtils.h"

static const FName StoryFlowPinCategory(TEXT("StoryFlow"));

namespace
{
	static FStoryShotID MakeNextShotID(const UStorySceneAsset* _scene_asset)
	{
		if (IsInvalid(_scene_asset))
		{
			return FStoryShotID();
		}

		TSet<int32> used_indices;

		for (const TObjectPtr<UStorySceneNodeData>& shot_node : _scene_asset->GetShotNodes())
		{
			if (IsInvalid(shot_node) || shot_node->GetShotID().IsValid() == false)
			{
				continue;
			}

			const FString shot_id_string = shot_node->GetShotID().Get().ToString();
			FString numeric_suffix;
			if (shot_id_string.Split(TEXT("Shot_"), nullptr, &numeric_suffix))
			{
				used_indices.Add(FCString::Atoi(*numeric_suffix));
			}
		}

		int32 next_index = 1;
		while (used_indices.Contains(next_index))
		{
			++next_index;
		}

		return FStoryShotID(*FString::Printf(TEXT("Shot_%03d"), next_index));
	}
}

void UStorySceneGraphNode_Shot::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, StoryFlowPinCategory, NAME_None, TEXT("In"));
	CreatePin(EGPD_Output, StoryFlowPinCategory, NAME_None, TEXT("Next"));
}

void UStorySceneGraphNode_Shot::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	if (IsValid(_ShotNodeData))
	{
		return;
	}

	UStorySceneAsset* scene_asset = GetOwningSceneAsset();
	if (IsInvalid(scene_asset))
	{
		return;
	}

	_ShotNodeData = scene_asset->CreateShotNode();
	if (IsValid(_ShotNodeData))
	{
		_ShotNodeData->SetShotID(MakeNextShotID(scene_asset));
	}
}

void UStorySceneGraphNode_Shot::DestroyNode()
{
	if (UStorySceneAsset* scene_asset = GetOwningSceneAsset())
	{
		scene_asset->RemoveShotNode(_ShotNodeData);
	}

	_ShotNodeData = nullptr;
	Super::DestroyNode();
}

void UStorySceneGraphNode_Shot::AutowireNewNode(UEdGraphPin* _from_pin)
{
	Super::AutowireNewNode(_from_pin);

	if (_from_pin == nullptr)
	{
		return;
	}

	const UEdGraphSchema* schema = GetSchema();
	if (schema == nullptr)
	{
		return;
	}

	UEdGraphPin* target_pin = (_from_pin->Direction == EGPD_Output) ? FindPin(TEXT("In")) : FindPin(TEXT("Next"));
	if (target_pin == nullptr)
	{
		return;
	}

	if (schema->TryCreateConnection(_from_pin, target_pin) == false)
	{
		schema->TryCreateConnection(target_pin, _from_pin);
	}

	if (UEdGraph* graph = GetGraph())
	{
		graph->NotifyGraphChanged();
	}
}

FText UStorySceneGraphNode_Shot::GetNodeTitle(ENodeTitleType::Type _title_type) const
{
	if (IsValid(_ShotNodeData))
	{
		const FText display_name = _ShotNodeData->GetDisplayNameText();
		const FText shot_id_text = _ShotNodeData->GetShotID().IsValid()
			? FText::FromName(_ShotNodeData->GetShotID().Get())
			: FText::FromString(TEXT("ShotID: None"));

		if (_title_type == ENodeTitleType::FullTitle || _title_type == ENodeTitleType::EditableTitle)
		{
			return FText::Format(FText::FromString(TEXT("{0}\n{1}")), display_name, shot_id_text);
		}

		return display_name;
	}

	return FText::FromString(TEXT("Shot"));
}

FText UStorySceneGraphNode_Shot::GetTooltipText() const
{
	return Super::GetTooltipText();
}

void UStorySceneGraphNode_Shot::ClearCompileMessage()
{
	bHasCompilerMessage = false;
	ErrorType = 0;
	ErrorMsg.Empty();
}

void UStorySceneGraphNode_Shot::SetCompileError(const FString& _error_message)
{
	bHasCompilerMessage = true;
	ErrorType = 1;
	ErrorMsg = _error_message;
}

UStorySceneAsset* UStorySceneGraphNode_Shot::GetOwningSceneAsset() const
{
	return GetGraph() ? Cast<UStorySceneAsset>(GetGraph()->GetOuter()) : nullptr;
}
