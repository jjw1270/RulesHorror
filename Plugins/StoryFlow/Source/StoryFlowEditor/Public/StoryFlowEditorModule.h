// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "IPIEAuthorizer.h"

class FStorySceneAssetTypeActions;
class FSlateStyleSet;
struct FGraphPanelNodeFactory;
class UStorySceneAsset;
class UStorySceneGraphNode_Shot;
class UStorySceneNodeData;

class STORYFLOWEDITOR_API FStoryFlowEditorModule : public IModuleInterface
{
private:
	TSharedPtr<FStorySceneAssetTypeActions> _StorySceneAssetTypeActions;
	TSharedPtr<FSlateStyleSet> _StyleSet;
	TUniquePtr<IPIEAuthorizer> _PIEAuthorizer;
	TSharedPtr<FGraphPanelNodeFactory> _GraphNodeFactory;
	FDelegateHandle _PostPIEStartedHandle;
	FDelegateHandle _CancelPIEHandle;

	TWeakObjectPtr<UStorySceneAsset> _PendingPlaySceneAsset;
	TWeakObjectPtr<UStorySceneNodeData> _PendingPlayShotNodeData;

public:
	static FStoryFlowEditorModule& Get();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RequestPlayFromShotNode(UStorySceneGraphNode_Shot* _shot_graph_node);
	void ClearPendingPlayFromShotRequest();

private:
	void OnPostPIEStarted(bool _is_simulating);
	void OnCancelPIE();
};
