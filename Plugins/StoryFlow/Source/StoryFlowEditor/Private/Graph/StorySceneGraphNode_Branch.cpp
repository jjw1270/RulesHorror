// Copyright (c) 2026 장윤제. All rights reserved.

#include "Graph/StorySceneGraphNode_Branch.h"
#include "StoryBranchNodeData.h"
#include "StorySceneAsset.h"
#include "StoryFlowDefines.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "CommonUtils.h"

namespace
{
	static const FName StorySceneBranchPinCategory(TEXT("StoryFlow"));

	static FStoryBranchID MakeNextBranchID(const UStorySceneAsset* _scene_asset)
	{
		if (IsInvalid(_scene_asset))
		{
			return FStoryBranchID();
		}

		TSet<int32> used_indices;
		for (const TObjectPtr<UStoryBranchNodeData>& branch_node : _scene_asset->GetBranchNodes())
		{
			if (IsInvalid(branch_node) || branch_node->GetBranchID().IsValid() == false)
			{
				continue;
			}

			const FString id_string = branch_node->GetBranchID().Get().ToString();
			FString numeric_suffix;
			if (id_string.Split(TEXT("Branch_"), nullptr, &numeric_suffix))
			{
				used_indices.Add(FCString::Atoi(*numeric_suffix));
			}
		}

		int32 next_index = 1;
		while (used_indices.Contains(next_index))
		{
			++next_index;
		}

		return FStoryBranchID(*FString::Printf(TEXT("Branch_%03d"), next_index));
	}
}

void UStorySceneGraphNode_Branch::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, StorySceneBranchPinCategory, NAME_None, TEXT("In"));

	const int32 next_pin_count = IsValid(_BranchNodeData) ? FMath::Max(_BranchNodeData->GetBranchCount(), 1) : 1;
	for (int32 index = 0; index < next_pin_count; ++index)
	{
		CreatePin(EGPD_Output, StorySceneBranchPinCategory, NAME_None, *FString::Printf(TEXT("Next_%d"), index));
	}
}

void UStorySceneGraphNode_Branch::PostLoad()
{
	Super::PostLoad();
	SyncNextPinsToNodeData();
}

void UStorySceneGraphNode_Branch::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	if (IsValid(_BranchNodeData))
	{
		return;
	}

	UStorySceneAsset* scene_asset = GetOwningSceneAsset();
	if (IsInvalid(scene_asset))
	{
		return;
	}

	_BranchNodeData = scene_asset->CreateBranchNode();
	if (IsValid(_BranchNodeData))
	{
		_BranchNodeData->SetBranchID(MakeNextBranchID(scene_asset));
	}
}

void UStorySceneGraphNode_Branch::DestroyNode()
{
	if (UStorySceneAsset* scene_asset = GetOwningSceneAsset())
	{
		scene_asset->RemoveBranchNode(_BranchNodeData);
	}

	_BranchNodeData = nullptr;
	Super::DestroyNode();
}

void UStorySceneGraphNode_Branch::AutowireNewNode(UEdGraphPin* _from_pin)
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

	UEdGraphPin* target_pin = (_from_pin->Direction == EGPD_Output) ? FindPin(TEXT("In")) : FindFirstAvailableNextPin();
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

FText UStorySceneGraphNode_Branch::GetNodeTitle(ENodeTitleType::Type _title_type) const
{
	if (IsValid(_BranchNodeData))
	{
		const FText display_name = _BranchNodeData->GetDisplayNameText();
		const FText branch_id_text = _BranchNodeData->GetBranchID().IsValid()
			? FText::FromName(_BranchNodeData->GetBranchID().Get())
			: FText::FromString(TEXT("BranchID: None"));

		if (_title_type == ENodeTitleType::FullTitle || _title_type == ENodeTitleType::EditableTitle)
		{
			return FText::Format(FText::FromString(TEXT("{0}\n{1}")), display_name, branch_id_text);
		}

		return display_name;
	}

	return FText::FromString(TEXT("Branch"));
}

FText UStorySceneGraphNode_Branch::GetTooltipText() const
{
	if (IsValid(_BranchNodeData))
	{
		const FText description = _BranchNodeData->GetDescriptionText();
		if (description.IsEmpty() == false)
		{
			return description;
		}
	}

	return Super::GetTooltipText();
}

UStorySceneAsset* UStorySceneGraphNode_Branch::GetOwningSceneAsset() const
{
	return GetGraph() ? Cast<UStorySceneAsset>(GetGraph()->GetOuter()) : nullptr;
}

TArray<UEdGraphPin*> UStorySceneGraphNode_Branch::GetNextPins() const
{
	TArray<UEdGraphPin*> next_pins;
	for (UEdGraphPin* pin : Pins)
	{
		if (pin && pin->Direction == EGPD_Output && pin->PinName.ToString().StartsWith(TEXT("Next_")))
		{
			next_pins.Add(pin);
		}
	}

	next_pins.Sort([](const UEdGraphPin& _lhs, const UEdGraphPin& _rhs)
		{
			return _lhs.PinName.LexicalLess(_rhs.PinName);
		});

	return next_pins;
}

UEdGraphPin* UStorySceneGraphNode_Branch::FindFirstAvailableNextPin() const
{
	for (UEdGraphPin* next_pin : GetNextPins())
	{
		if (next_pin && next_pin->LinkedTo.Num() == 0)
		{
			return next_pin;
		}
	}

	TArray<UEdGraphPin*> next_pins = GetNextPins();
	return next_pins.Num() > 0 ? next_pins[0] : nullptr;
}

void UStorySceneGraphNode_Branch::SyncNextPinsToNodeData()
{
	const int32 desired_branch_count = IsValid(_BranchNodeData) ? FMath::Max(_BranchNodeData->GetBranchCount(), 1) : 1;
	TArray<UEdGraphPin*> next_pins = GetNextPins();
	const int32 current_branch_count = next_pins.Num();
	if (desired_branch_count == current_branch_count)
	{
		return;
	}

	Modify();
	if (desired_branch_count > current_branch_count)
	{
		for (int32 index = current_branch_count; index < desired_branch_count; ++index)
		{
			CreatePin(EGPD_Output, StorySceneBranchPinCategory, NAME_None, *FString::Printf(TEXT("Next_%d"), index));
		}
	}
	else
	{
		for (int32 index = current_branch_count - 1; index >= desired_branch_count; --index)
		{
			UEdGraphPin* pin_to_remove = next_pins.IsValidIndex(index) ? next_pins[index] : nullptr;
			if (pin_to_remove)
			{
				pin_to_remove->BreakAllPinLinks();
				RemovePin(pin_to_remove);
			}
		}
	}

	if (UEdGraph* graph = GetGraph())
	{
		graph->NotifyGraphChanged();
	}
}

void UStorySceneGraphNode_Branch::ClearCompileMessage()
{
	bHasCompilerMessage = false;
	ErrorType = 0;
	ErrorMsg.Empty();
}

void UStorySceneGraphNode_Branch::SetCompileError(const FString& _error_message)
{
	bHasCompilerMessage = true;
	ErrorType = 1;
	ErrorMsg = _error_message;
}
