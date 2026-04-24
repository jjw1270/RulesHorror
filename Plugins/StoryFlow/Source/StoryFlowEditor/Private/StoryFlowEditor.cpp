// Copyright (c) 2026 장윤제. All rights reserved.

#include "StoryFlowEditorModule.h"
#include "StorySceneAssetTypeActions.h"
#include "StorySceneEditor.h"
#include "StorySceneAsset.h"
#include "StorySceneNodeData.h"
#include "StoryFlowSubsystem.h"
#include "Graph/SStorySceneGraphNode_Shot.h"
#include "Graph/StorySceneGraphNode_Shot.h"
#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "Features/IModularFeatures.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "PlayInEditorDataTypes.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/GameInstance.h"
#include "CommonUtils.h"

IMPLEMENT_MODULE(FStoryFlowEditorModule, StoryFlowEditor)

static const FName StyleSetName(TEXT("StoryFlowEditorStyle"));

namespace
{
	class FStoryFlowGraphNodeFactory : public FGraphPanelNodeFactory
	{
	public:
		virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* _node) const override
		{
			UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(_node);
			if (IsValid(shot_node))
			{
				return SNew(SStorySceneGraphNode_Shot)
					.GraphNodeObj(shot_node);
			}

			return nullptr;
		}
	};

	class FStoryFlowPIEAuthorizer : public IPIEAuthorizer
	{
	public:
		virtual bool RequestPIEPermission(bool _is_simulate_in_editor, FString& _out_reason) const override
		{
			TArray<FStorySceneEditor*> active_editors;
			FStorySceneEditor::GetActiveEditors(active_editors);
			if (active_editors.Num() == 0)
			{
				return true;
			}

			FStorySceneEditor::ClearCompileMessageLog();

			bool is_all_valid = true;
			TArray<FString> denied_reasons;

			for (FStorySceneEditor* editor : active_editors)
			{
				if (editor == nullptr)
				{
					continue;
				}

				FString denied_reason;
				if (editor->ValidateForPIE(denied_reason) == false)
				{
					is_all_valid = false;
					denied_reasons.Add(denied_reason);
				}
			}

			if (is_all_valid == false)
			{
				FStoryFlowEditorModule::Get().ClearPendingPlayFromShotRequest();
				_out_reason = FString::Join(denied_reasons, TEXT("\n"));
			}

			return is_all_valid;
		}
	};
}

FStoryFlowEditorModule& FStoryFlowEditorModule::Get()
{
	return FModuleManager::LoadModuleChecked<FStoryFlowEditorModule>("StoryFlowEditor");
}

void FStoryFlowEditorModule::StartupModule()
{
	IAssetTools& asset_tools = FAssetToolsModule::GetModule().Get();
	const EAssetTypeCategories::Type category = asset_tools.RegisterAdvancedAssetCategory(FName("StoryFlow"), FText::FromString(TEXT("StoryFlow")));

	_StorySceneAssetTypeActions = MakeShared<FStorySceneAssetTypeActions>(category);
	asset_tools.RegisterAssetTypeActions(_StorySceneAssetTypeActions.ToSharedRef());

	_StyleSet = MakeShared<FSlateStyleSet>(StyleSetName);
	const FSlateBrush* icon = FAppStyle::GetBrush(TEXT("ClassIcon.Blueprint"));
	const FSlateBrush* thumbnail = FAppStyle::GetBrush(TEXT("ClassThumbnail.Blueprint"));

	_StyleSet->Set(TEXT("ClassIcon.StorySceneAsset"), new FSlateBrush(*icon));
	_StyleSet->Set(TEXT("ClassThumbnail.StorySceneAsset"), new FSlateBrush(*thumbnail));

	if (FSlateStyleRegistry::FindSlateStyle(StyleSetName) != nullptr)
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(StyleSetName);
	}

	FSlateStyleRegistry::RegisterSlateStyle(*_StyleSet);

	_PIEAuthorizer = MakeUnique<FStoryFlowPIEAuthorizer>();
	IModularFeatures::Get().RegisterModularFeature(IPIEAuthorizer::GetModularFeatureName(), _PIEAuthorizer.Get());

	_GraphNodeFactory = MakeShared<FStoryFlowGraphNodeFactory>();
	FEdGraphUtilities::RegisterVisualNodeFactory(_GraphNodeFactory);

	_PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FStoryFlowEditorModule::OnPostPIEStarted);
	_CancelPIEHandle = FEditorDelegates::CancelPIE.AddRaw(this, &FStoryFlowEditorModule::OnCancelPIE);
}

void FStoryFlowEditorModule::ShutdownModule()
{
	if (_CancelPIEHandle.IsValid())
	{
		FEditorDelegates::CancelPIE.Remove(_CancelPIEHandle);
		_CancelPIEHandle.Reset();
	}

	if (_PostPIEStartedHandle.IsValid())
	{
		FEditorDelegates::PostPIEStarted.Remove(_PostPIEStartedHandle);
		_PostPIEStartedHandle.Reset();
	}

	if (_GraphNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(_GraphNodeFactory);
		_GraphNodeFactory.Reset();
	}

	if (_PIEAuthorizer.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(IPIEAuthorizer::GetModularFeatureName(), _PIEAuthorizer.Get());
		_PIEAuthorizer.Reset();
	}

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& asset_tools = FAssetToolsModule::GetModule().Get();
		if (_StorySceneAssetTypeActions.IsValid())
		{
			asset_tools.UnregisterAssetTypeActions(_StorySceneAssetTypeActions.ToSharedRef());
			_StorySceneAssetTypeActions.Reset();
		}
	}

	if (_StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*_StyleSet);
		_StyleSet.Reset();
	}
}

void FStoryFlowEditorModule::RequestPlayFromShotNode(UStorySceneGraphNode_Shot* _shot_graph_node)
{
	if (IsInvalid(_shot_graph_node) || IsInvalid(_shot_graph_node->GetShotNodeData()) || IsInvalid(_shot_graph_node->GetOwningSceneAsset()))
	{
		EDITOR_NOTIFY_WARNING(TEXT("Shot 플레이를 시작할 수 없습니다."));
		return;
	}

	if (IsValid(GEditor) && IsValid(GEditor->PlayWorld))
	{
		EDITOR_NOTIFY_WARNING(TEXT("현재 PIE가 실행 중입니다. 먼저 종료해주세요."));
		return;
	}

	_PendingPlaySceneAsset = _shot_graph_node->GetOwningSceneAsset();
	_PendingPlayShotNodeData = _shot_graph_node->GetShotNodeData();

	if (IsInvalid(GEditor))
	{
		ClearPendingPlayFromShotRequest();
		return;
	}

	FRequestPlaySessionParams session_params;
	GEditor->RequestPlaySession(session_params);
}

void FStoryFlowEditorModule::ClearPendingPlayFromShotRequest()
{
	_PendingPlaySceneAsset = nullptr;
	_PendingPlayShotNodeData = nullptr;
}

void FStoryFlowEditorModule::OnPostPIEStarted(bool _is_simulating)
{
	if (IsAnyInvalid(_PendingPlaySceneAsset, _PendingPlayShotNodeData))
	{
		ClearPendingPlayFromShotRequest();
		return;
	}

	FStoryFlowRef story_flow_ref;
	story_flow_ref.SceneID = _PendingPlaySceneAsset->GetSceneID();
	story_flow_ref.ShotID = _PendingPlayShotNodeData->GetShotID();

	ClearPendingPlayFromShotRequest();

	if (story_flow_ref.IsValid() == false || story_flow_ref.ShotID.IsValid() == false)
	{
		EDITOR_NOTIFY_WARNING(TEXT("선택한 Shot으로 시작할 수 없습니다."));
		return;
	}

	UWorld* play_world = IsValid(GEditor) ? GEditor->PlayWorld : nullptr;
	UGameInstance* game_instance = IsValid(play_world) ? play_world->GetGameInstance() : nullptr;
	UStoryFlowSubsystem* story_flow_subsystem = IsValid(game_instance) ? game_instance->GetSubsystem<UStoryFlowSubsystem>() : nullptr;
	if (IsInvalid(story_flow_subsystem) || story_flow_subsystem->StartFromRef(story_flow_ref) == false)
	{
		EDITOR_NOTIFY_ERROR(TEXT("선택한 Shot에서 StoryFlow를 시작하지 못했습니다."));
	}
}

void FStoryFlowEditorModule::OnCancelPIE()
{
	ClearPendingPlayFromShotRequest();
}
