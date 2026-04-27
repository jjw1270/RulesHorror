// Copyright (c) 2026 장윤제. All rights reserved.

#include "StorySceneEditor.h"
#include "StorySceneAsset.h"
#include "StoryBranchNodeData.h"
#include "StorySceneNodeData.h"
#include "StoryFlowDeveloperSettings.h"
#include "StorySceneRegistryAsset.h"
#include "Graph/StorySceneEdGraph.h"
#include "Graph/StorySceneGraphNode_Entry.h"
#include "Graph/StorySceneGraphNode_Branch.h"
#include "Graph/StorySceneGraphSchema.h"
#include "Graph/StorySceneGraphNode_Shot.h"
#include "Graph/StorySceneGraphNode_Transition.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "GraphEditor.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "ScopedTransaction.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Editor/Transactor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/PackageName.h"
#include "Styling/AppStyle.h"
#include "CommonUtils.h"

const FName FStorySceneEditor::_GraphTabID(TEXT("StorySceneEditor_Graph"));
const FName FStorySceneEditor::_DetailsTabID(TEXT("StorySceneEditor_Details"));
const FName FStorySceneEditor::_CompileMessageLogName(TEXT("StoryFlow"));
TArray<FStorySceneEditor*> FStorySceneEditor::_ActiveEditors;

namespace
{
	static FStorySceneID MakeDefaultSceneID(const UStorySceneAsset* _scene_asset)
	{
		if (IsInvalid(_scene_asset))
		{
			return FStorySceneID();
		}

		return FStorySceneID(_scene_asset->GetFName());
	}

	static FStoryShotID MakeNextShotID(UStorySceneEdGraph* _graph, const TSet<FName>* _existing_ids = nullptr)
	{
		if (IsInvalid(_graph))
		{
			return FStoryShotID();
		}

		TSet<int32> used_indices;

		if (_existing_ids)
		{
			for (const FName& existing_id : *_existing_ids)
			{
				const FString id_string = existing_id.ToString();
				FString numeric_suffix;
				if (id_string.Split(TEXT("Shot_"), nullptr, &numeric_suffix))
				{
					used_indices.Add(FCString::Atoi(*numeric_suffix));
				}
			}
		}

		if (const UStorySceneAsset* scene_asset = _graph->GetOwningSceneAsset())
		{
			for (const TObjectPtr<UStorySceneNodeData>& shot_node : scene_asset->GetShotNodes())
			{
				if (IsInvalid(shot_node) || shot_node->GetShotID().IsValid() == false)
				{
					continue;
				}

				const FString id_string = shot_node->GetShotID().Get().ToString();
				FString numeric_suffix;
				if (id_string.Split(TEXT("Shot_"), nullptr, &numeric_suffix))
				{
					used_indices.Add(FCString::Atoi(*numeric_suffix));
				}
			}
		}

		int32 next_index = 1;
		while (used_indices.Contains(next_index))
		{
			++next_index;
		}

		return FStoryShotID(*FString::Printf(TEXT("Shot_%03d"), next_index));
	}

	static int32 CountSceneRegistryMatches(const UStorySceneRegistryAsset* _scene_registry, const FStorySceneID& _scene_id)
	{
		if (IsInvalid(_scene_registry) || _scene_id.IsValid() == false)
		{
			return 0;
		}

		int32 match_count = 0;
		for (const FStorySceneReference& scene_reference : _scene_registry->GetScenes())
		{
			if (scene_reference.GetSceneID() == _scene_id)
			{
				++match_count;
			}
		}

		return match_count;
	}

	static FString GetLevelPackageName(const TSoftObjectPtr<UWorld>& _level)
	{
		if (_level.IsNull())
		{
			return FString();
		}

		return FPackageName::ObjectPathToPackageName(_level.ToSoftObjectPath().ToString());
	}
}

FStorySceneEditor::~FStorySceneEditor()
{
	_ActiveEditors.Remove(this);

	if (_ObservedGraph.IsValid() && _OnGraphChangedHandle.IsValid())
	{
		_ObservedGraph->RemoveOnGraphChangedHandler(_OnGraphChangedHandle);
	}

	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

void FStorySceneEditor::GetActiveEditors(TArray<FStorySceneEditor*>& _out_editors)
{
	_ActiveEditors.RemoveAll([](const FStorySceneEditor* _editor)
		{
			return _editor == nullptr;
		});

	_out_editors = _ActiveEditors;
}

void FStorySceneEditor::ClearCompileMessageLog()
{
	EDITOR_MESSAGE_CLEAR(_CompileMessageLogName);
}

void FStorySceneEditor::InitEditor(const EToolkitMode::Type _mode, const TSharedPtr<IToolkitHost>& _toolkit_host, UStorySceneAsset* _story_scene_asset)
{
	_StorySceneAsset = _story_scene_asset;
	_ActiveEditors.AddUnique(this);

	const TSharedRef<FTabManager::FLayout> layout =
		FTabManager::NewLayout("StorySceneEditor_Layout_v1")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->AddTab(_GraphTabID, ETabState::OpenedTab)
				->SetHideTabWell(true)
				->SetSizeCoefficient(0.7f))
			->Split(
				FTabManager::NewStack()
				->AddTab(_DetailsTabID, ETabState::OpenedTab)
				->SetSizeCoefficient(0.3f)));

	FAssetEditorToolkit::InitAssetEditor(_mode, _toolkit_host, GetToolkitFName(), layout, true, true, _story_scene_asset);

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	if (_EditorCommands.IsValid() == false)
	{
		BindEditorCommands();
	}

	TSharedRef<FExtender> toolbar_extender = MakeShared<FExtender>();
	toolbar_extender->AddToolBarExtension(
		TEXT("Asset"),
		EExtensionHook::After,
		_EditorCommands,
		FToolBarExtensionDelegate::CreateSP(this, &FStorySceneEditor::FillToolbar));
	AddToolbarExtender(toolbar_extender);

	RegenerateMenusAndToolbars();
	CompileSceneInternal();
}

void FStorySceneEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& _tab_manager)
{
	FAssetEditorToolkit::RegisterTabSpawners(_tab_manager);

	_WorkspaceMenuCategory = _tab_manager->AddLocalWorkspaceMenuCategory(FText::FromString(TEXT("Story Scene")));

	_tab_manager->RegisterTabSpawner(_GraphTabID, FOnSpawnTab::CreateSP(this, &FStorySceneEditor::SpawnGraphTab))
		.SetDisplayName(FText::FromString(TEXT("Graph")))
		.SetGroup(_WorkspaceMenuCategory.ToSharedRef());

	_tab_manager->RegisterTabSpawner(_DetailsTabID, FOnSpawnTab::CreateSP(this, &FStorySceneEditor::SpawnDetailsTab))
		.SetDisplayName(FText::FromString(TEXT("Details")))
		.SetGroup(_WorkspaceMenuCategory.ToSharedRef());
}

void FStorySceneEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& _tab_manager)
{
	_tab_manager->UnregisterTabSpawner(_GraphTabID);
	_tab_manager->UnregisterTabSpawner(_DetailsTabID);

	FAssetEditorToolkit::UnregisterTabSpawners(_tab_manager);
}

void FStorySceneEditor::PostUndo(bool _is_success)
{
	if (_is_success && _GraphEditorWidget.IsValid())
	{
		_GraphEditorWidget->NotifyGraphChanged();
	}
}

TSharedRef<SDockTab> FStorySceneEditor::SpawnGraphTab(const FSpawnTabArgs& _args)
{
	UStorySceneEdGraph* graph = GetOrCreateGraph();
	SyncGraphNodePins(graph);

	if (_GraphEditorCommands.IsValid() == false)
	{
		BindGraphCommands();
	}

	SGraphEditor::FGraphEditorEvents graph_editor_events;
	graph_editor_events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FStorySceneEditor::OnGraphSelectionChanged);

	if (_ObservedGraph.Get() != graph)
	{
		if (_ObservedGraph.IsValid() && _OnGraphChangedHandle.IsValid())
		{
			_ObservedGraph->RemoveOnGraphChangedHandler(_OnGraphChangedHandle);
			_OnGraphChangedHandle.Reset();
		}

		_ObservedGraph = graph;
		_OnGraphChangedHandle = graph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FStorySceneEditor::OnGraphChanged));
	}

	_GraphEditorWidget = SNew(SGraphEditor)
		.IsEditable(true)
		.AdditionalCommands(_GraphEditorCommands)
		.GraphToEdit(graph)
		.GraphEvents(graph_editor_events)
		.ShowGraphStateOverlay(false);

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			_GraphEditorWidget.ToSharedRef()
		];
}

void FStorySceneEditor::FillToolbar(FToolBarBuilder& _toolbar_builder)
{
	_toolbar_builder.BeginSection(TEXT("StorySceneCompile"));
	{
		_toolbar_builder.AddWidget(BuildCompileToolbarWidget());
	}
	_toolbar_builder.EndSection();
}

TSharedRef<SWidget> FStorySceneEditor::BuildCompileToolbarWidget()
{
	return SNew(SButton)
		.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
		.ContentPadding(FMargin(6.0f, 2.0f))
		.ToolTipText(this, &FStorySceneEditor::GetCompileButtonTooltipText)
		.OnClicked(this, &FStorySceneEditor::OnClickCompileButton)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(this, &FStorySceneEditor::GetCompileBaseBrush)
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.Image(this, &FStorySceneEditor::GetCompileStatusBrush)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Compile")))
			]
		];
}

FReply FStorySceneEditor::OnClickCompileButton()
{
	CompileScene();
	return FReply::Handled();
}

const FSlateBrush* FStorySceneEditor::GetCompileBaseBrush() const
{
	return FAppStyle::GetBrush(TEXT("Blueprint.CompileStatus.Background"));
}

const FSlateBrush* FStorySceneEditor::GetCompileStatusBrush() const
{
	switch (_CompileStatus)
	{
	case ECompileStatus::Good:
		return FAppStyle::GetBrush(TEXT("Blueprint.CompileStatus.Overlay.Good"));
	case ECompileStatus::Error:
		return FAppStyle::GetBrush(TEXT("Blueprint.CompileStatus.Overlay.Error"));
	case ECompileStatus::Dirty:
	default:
		return FAppStyle::GetBrush(TEXT("Blueprint.CompileStatus.Overlay.Unknown"));
	}
}

FText FStorySceneEditor::GetCompileButtonTooltipText() const
{
	switch (_CompileStatus)
	{
	case ECompileStatus::Good:
		return FText::FromString(TEXT("Compiled successfully."));
	case ECompileStatus::Error:
		return FText::FromString(_CompileErrors.Num() > 0
			? FString::Join(_CompileErrors, TEXT("\n"))
			: TEXT("Compile failed."));
	case ECompileStatus::Dirty:
	default:
		return FText::FromString(TEXT("Graph has changed and needs to be compiled."));
	}
}

TSharedRef<SDockTab> FStorySceneEditor::SpawnDetailsTab(const FSpawnTabArgs& _args)
{
	FPropertyEditorModule& property_editor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs details_view_args;
	details_view_args.bUpdatesFromSelection = false;
	details_view_args.bHideSelectionTip = true;

	_DetailsView = property_editor.CreateDetailView(details_view_args);
	_DetailsView->SetObject(_StorySceneAsset.Get());
	_DetailsView->OnFinishedChangingProperties().AddSP(this, &FStorySceneEditor::OnDetailsFinishedChangingProperties);

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			_DetailsView.ToSharedRef()
		];
}

void FStorySceneEditor::OnGraphSelectionChanged(const TSet<UObject*>& _selection)
{
	if (_DetailsView.IsValid() == false)
	{
		return;
	}

	for (UObject* selected_object : _selection)
	{
		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(selected_object);
		if (IsValid(shot_node) && IsValid(shot_node->GetShotNodeData()))
		{
			_DetailsView->SetObject(shot_node->GetShotNodeData());
			return;
		}

		UStorySceneGraphNode_Branch* branch_node = Cast<UStorySceneGraphNode_Branch>(selected_object);
		if (IsValid(branch_node) && IsValid(branch_node->GetBranchNodeData()))
		{
			_DetailsView->SetObject(branch_node->GetBranchNodeData());
			return;
		}

		UStorySceneGraphNode_Transition* transition_node = Cast<UStorySceneGraphNode_Transition>(selected_object);
		if (IsValid(transition_node))
		{
			_DetailsView->SetObject(transition_node);
			return;
		}
	}

	_DetailsView->SetObject(_StorySceneAsset.Get());
}

void FStorySceneEditor::OnDetailsFinishedChangingProperties(const FPropertyChangedEvent& _property_changed_event)
{
	if (_IsCompiling)
	{
		return;
	}

	if (_GraphEditorWidget.IsValid())
	{
		if (UStorySceneEdGraph* graph = Cast<UStorySceneEdGraph>(_GraphEditorWidget->GetCurrentGraph()))
		{
			SyncGraphNodePins(graph);
		}
	}

	MarkCompileDirty();

	if (_GraphEditorWidget.IsValid())
	{
		_GraphEditorWidget->NotifyGraphChanged();
	}
}

void FStorySceneEditor::OnGraphChanged(const FEdGraphEditAction& _graph_edit_action)
{
	if (_IsCompiling)
	{
		return;
	}

	MarkCompileDirty();
}

void FStorySceneEditor::BindGraphCommands()
{
	_GraphEditorCommands = MakeShared<FUICommandList>();

	_GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FStorySceneEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FStorySceneEditor::CanDeleteSelectedNodes));

	_GraphEditorCommands->MapAction(
		FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FStorySceneEditor::CopySelectedNodes),
		FCanExecuteAction::CreateSP(this, &FStorySceneEditor::CanCopySelectedNodes));

	_GraphEditorCommands->MapAction(
		FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &FStorySceneEditor::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FStorySceneEditor::CanCutSelectedNodes));

	_GraphEditorCommands->MapAction(
		FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &FStorySceneEditor::PasteNodes),
		FCanExecuteAction::CreateSP(this, &FStorySceneEditor::CanPasteNodes));

	_GraphEditorCommands->MapAction(
		FGenericCommands::Get().Undo,
		FExecuteAction::CreateSP(this, &FStorySceneEditor::UndoGraphAction),
		FCanExecuteAction::CreateSP(this, &FStorySceneEditor::CanUndoGraphAction));

	_GraphEditorCommands->MapAction(
		FGenericCommands::Get().Redo,
		FExecuteAction::CreateSP(this, &FStorySceneEditor::RedoGraphAction),
		FCanExecuteAction::CreateSP(this, &FStorySceneEditor::CanRedoGraphAction));
}

void FStorySceneEditor::BindEditorCommands()
{
	_EditorCommands = MakeShared<FUICommandList>();
}

void FStorySceneEditor::CompileScene()
{
	CompileSceneInternal();
}

void FStorySceneEditor::CopySelectedNodes()
{
	if (IsInvalid(_GraphEditorWidget))
	{
		return;
	}

	const FGraphPanelSelectionSet selected_nodes = _GraphEditorWidget->GetSelectedNodes();
	FGraphPanelSelectionSet copyable_nodes;

	for (UObject* selected_object : selected_nodes)
	{
		UEdGraphNode* selected_node = Cast<UEdGraphNode>(selected_object);
		if (IsInvalid(selected_node) || selected_node->CanDuplicateNode() == false)
		{
			continue;
		}

		selected_node->PrepareForCopying();
		copyable_nodes.Add(selected_node);
	}

	if (copyable_nodes.IsEmpty())
	{
		return;
	}

	FString exported_text;
	FEdGraphUtilities::ExportNodesToText(copyable_nodes, exported_text);
	FPlatformApplicationMisc::ClipboardCopy(*exported_text);

	for (UObject* copied_object : copyable_nodes)
	{
		if (UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(copied_object))
		{
			shot_node->PostCopyNode();
			continue;
		}

		if (UStorySceneGraphNode_Branch* branch_node = Cast<UStorySceneGraphNode_Branch>(copied_object))
		{
			branch_node->PostCopyNode();
		}
	}
}

bool FStorySceneEditor::CanCopySelectedNodes() const
{
	return CanDeleteSelectedNodes();
}

void FStorySceneEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedNodes();
}

bool FStorySceneEditor::CanCutSelectedNodes() const
{
	return CanDeleteSelectedNodes() && CanCopySelectedNodes();
}

void FStorySceneEditor::PasteNodes()
{
	if (IsInvalid(_GraphEditorWidget))
	{
		return;
	}

	PasteNodesHere(FVector2D(_GraphEditorWidget->GetPasteLocation2f()));
}

bool FStorySceneEditor::CanPasteNodes() const
{
	if (IsInvalid(_GraphEditorWidget))
	{
		return false;
	}

	FString clipboard_content;
	FPlatformApplicationMisc::ClipboardPaste(clipboard_content);
	return FEdGraphUtilities::CanImportNodesFromText(_GraphEditorWidget->GetCurrentGraph(), clipboard_content);
}

void FStorySceneEditor::PasteNodesHere(const FVector2D& _location)
{
	if (IsInvalid(_GraphEditorWidget))
	{
		return;
	}

	UEdGraph* graph = _GraphEditorWidget->GetCurrentGraph();
	if (IsInvalid(graph))
	{
		return;
	}

	const FScopedTransaction transaction(FGenericCommands::Get().Paste->GetDescription());
	graph->Modify();
	_GraphEditorWidget->ClearSelectionSet();

	FString text_to_import;
	FPlatformApplicationMisc::ClipboardPaste(text_to_import);

	TSet<UEdGraphNode*> pasted_nodes;
	FEdGraphUtilities::ImportNodesFromText(graph, text_to_import, pasted_nodes);
	if (pasted_nodes.IsEmpty())
	{
		return;
	}

	FVector2D average_node_position(0.0f, 0.0f);
	for (UEdGraphNode* pasted_node : pasted_nodes)
	{
		average_node_position.X += pasted_node->NodePosX;
		average_node_position.Y += pasted_node->NodePosY;
	}

	const double pasted_node_count = static_cast<double>(pasted_nodes.Num());
	average_node_position /= pasted_node_count;

	for (UEdGraphNode* pasted_node : pasted_nodes)
	{
		_GraphEditorWidget->SetNodeSelection(pasted_node, true);

		pasted_node->NodePosX = static_cast<int32>((pasted_node->NodePosX - average_node_position.X) + _location.X);
		pasted_node->NodePosY = static_cast<int32>((pasted_node->NodePosY - average_node_position.Y) + _location.Y);
		pasted_node->SnapToGrid(16);
		pasted_node->CreateNewGuid();
	}

	MarkCompileDirty();
	_GraphEditorWidget->NotifyGraphChanged();
}

bool FStorySceneEditor::ValidateForPIE(FString& _out_denied_reason)
{
	const bool is_compile_success = (_CompileStatus == ECompileStatus::Good) ? true : CompileSceneInternal();
	if (is_compile_success)
	{
		return true;
	}

	ReportCompileFailureForPIE();
	_out_denied_reason = IsValid(_StorySceneAsset)
		? FString::Printf(TEXT("StoryScene compile failed: %s"), *_StorySceneAsset->GetPathName())
		: TEXT("StoryScene compile failed.");
	return false;
}

bool FStorySceneEditor::CompileSceneInternal()
{
	UStorySceneEdGraph* graph = GetOrCreateGraph();
	if (IsInvalid(graph))
	{
		return false;
	}

	_IsCompiling = true;
	if (IsValid(_StorySceneAsset) && _StorySceneAsset->GetSceneID().IsValid() == false)
	{
		_StorySceneAsset->SetSceneID(MakeDefaultSceneID(_StorySceneAsset));
	}
	RefreshShotIDsForCompile(graph);
	RefreshShotNodeDescriptionsForCompile(graph);
	graph->RebuildRuntimeData();

	_CompileErrors.Reset();
	const bool is_success = ValidateCompiledScene(graph, _CompileErrors);
	_CompileStatus = is_success ? ECompileStatus::Good : ECompileStatus::Error;

	if (_GraphEditorWidget.IsValid())
	{
		_GraphEditorWidget->NotifyGraphChanged();
	}

	_IsCompiling = false;

	return is_success;
}

void FStorySceneEditor::ReportCompileFailureForPIE() const
{
	if (IsInvalid(_StorySceneAsset))
	{
		return;
	}

	EDITOR_MESSAGE_ERROR_OBJECT(_CompileMessageLogName, _StorySceneAsset.Get(), TEXT("PIE blocked because StoryScene compilation failed."));

	for (const FString& compile_error : _CompileErrors)
	{
		EDITOR_MESSAGE_ERROR_OBJECT(_CompileMessageLogName, _StorySceneAsset.Get(), TEXT("%s"), *compile_error);
	}

	EDITOR_NOTIFY_ERROR(TEXT("StoryScene compile failed. PIE was cancelled."));
}

void FStorySceneEditor::RefreshShotIDsForCompile(UStorySceneEdGraph* _graph) const
{
	if (IsInvalid(_graph))
	{
		return;
	}

	TSet<FName> used_shot_ids;

	for (UEdGraphNode* node : _graph->Nodes)
	{
		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsInvalid(shot_node) || IsInvalid(shot_node->GetShotNodeData()))
		{
			continue;
		}

		FStoryShotID shot_id = shot_node->GetShotNodeData()->GetShotID();
		if (shot_id.IsValid() == false || used_shot_ids.Contains(shot_id.Get()))
		{
			const FStoryShotID new_shot_id = MakeNextShotID(_graph, &used_shot_ids);
			shot_node->GetShotNodeData()->SetShotID(new_shot_id);
			shot_id = shot_node->GetShotNodeData()->GetShotID();
		}

		if (shot_id.IsValid())
		{
			used_shot_ids.Add(shot_id.Get());
		}
	}
}

void FStorySceneEditor::RefreshShotNodeDescriptionsForCompile(UStorySceneEdGraph* _graph) const
{
	if (IsInvalid(_graph))
	{
		return;
	}

	for (UEdGraphNode* node : _graph->Nodes)
	{
		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsInvalid(shot_node) || IsInvalid(shot_node->GetShotNodeData()))
		{
			continue;
		}

		const FString description = shot_node->GetShotNodeData()->GetDescriptionText().ToString();
		const bool has_description = description.IsEmpty() == false;

		shot_node->NodeComment = description;
		shot_node->bCommentBubbleVisible = has_description;
	}
}

bool FStorySceneEditor::ValidateCompiledScene(UStorySceneEdGraph* _graph, TArray<FString>& _out_errors)
{
	if (IsInvalid(_StorySceneAsset))
	{
		_out_errors.Add(TEXT("StorySceneAsset is invalid."));
		return false;
	}

	ClearNodeCompileMessages(_graph);
	ValidateSceneMetadata(_graph, _out_errors);

	TSet<FName> used_branch_ids;
	TSet<FName> used_shot_ids;
	TSet<UEdGraphNode*> reachable_nodes;
	_graph->GetReachableNodes(reachable_nodes);

	for (UEdGraphNode* node : reachable_nodes)
	{
		if (UStorySceneGraphNode_Branch* branch_node = Cast<UStorySceneGraphNode_Branch>(node))
		{
			ValidateBranchNode(branch_node, used_branch_ids, _out_errors);
			continue;
		}

		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsInvalid(shot_node))
		{
			if (UStorySceneGraphNode_Transition* transition_node = Cast<UStorySceneGraphNode_Transition>(node))
			{
				ValidateTransitionNode(transition_node, _out_errors);
			}
			continue;
		}
		ValidateShotNode(shot_node, used_shot_ids, _out_errors);
	}

	return _out_errors.Num() == 0;
}

void FStorySceneEditor::ValidateSceneMetadata(UStorySceneEdGraph* _graph, TArray<FString>& _out_errors) const
{
	const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();
	UStorySceneRegistryAsset* scene_registry = (IsValid(settings) && settings->_StorySceneRegistry.IsNull() == false)
		? settings->_StorySceneRegistry.LoadSynchronous()
		: nullptr;

	if (_StorySceneAsset->GetSceneID().IsValid() == false)
	{
		_out_errors.Add(TEXT("SceneID is empty."));
	}

	if (IsInvalid(settings) || settings->_LoadingLevel.IsNull())
	{
		_out_errors.Add(TEXT("LoadingLevel is missing in StoryFlowDeveloperSettings."));
	}

	if (_StorySceneAsset->GetTargetLevel().IsNull())
	{
		_out_errors.Add(TEXT("TargetLevel is missing."));
	}
	else if (IsValid(settings) && settings->_LoadingLevel.IsNull() == false
		&& GetLevelPackageName(settings->_LoadingLevel) == GetLevelPackageName(_StorySceneAsset->GetTargetLevel()))
	{
		_out_errors.Add(TEXT("LoadingLevel must be different from TargetLevel."));
	}

	if (IsInvalid(_StorySceneAsset->GetSceneTemplate()))
	{
		_out_errors.Add(TEXT("SceneTemplate is missing."));
	}

	if (_StorySceneAsset->GetSceneID().IsValid() && IsValid(scene_registry))
	{
		const int32 matching_scene_count = CountSceneRegistryMatches(scene_registry, _StorySceneAsset->GetSceneID());
		if (matching_scene_count == 0)
		{
			_out_errors.Add(FString::Printf(TEXT("SceneID '%s' is not registered in StorySceneRegistry."), *_StorySceneAsset->GetSceneID().Get().ToString()));
		}
		else if (matching_scene_count > 1)
		{
			_out_errors.Add(FString::Printf(TEXT("SceneID '%s' is duplicated in StorySceneRegistry."), *_StorySceneAsset->GetSceneID().Get().ToString()));
		}
	}

	if (_StorySceneAsset->GetEntryShotID().IsValid() == false)
	{
		_out_errors.Add(TEXT("Entry node is not connected to a Shot node."));

		if (UStorySceneGraphNode_Entry* entry_node = _graph->FindEntryNode())
		{
			entry_node->SetCompileError(TEXT("Entry node is not connected to a Shot node."));
		}
		return;
	}

	if (IsInvalid(_StorySceneAsset->FindShotNode(_StorySceneAsset->GetEntryShotID())))
	{
		_out_errors.Add(FString::Printf(TEXT("EntryShotID '%s' does not exist in ShotNodes."), *_StorySceneAsset->GetEntryShotID().Get().ToString()));

		if (UStorySceneGraphNode_Entry* entry_node = _graph->FindEntryNode())
		{
			entry_node->SetCompileError(TEXT("EntryShotID does not exist in ShotNodes."));
		}
	}
}

void FStorySceneEditor::ValidateBranchNode(UStorySceneGraphNode_Branch* _branch_node, TSet<FName>& _used_branch_ids, TArray<FString>& _out_errors) const
{
	if (IsInvalid(_branch_node))
	{
		return;
	}

	UStoryBranchNodeData* branch_node_data = _branch_node->GetBranchNodeData();
	if (IsInvalid(branch_node_data))
	{
		const FString error_message = TEXT("A Branch node does not have valid node data.");
		_out_errors.Add(error_message);
		_branch_node->SetCompileError(error_message);
		return;
	}

	TArray<FString> node_errors;
	const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();
	UStorySceneRegistryAsset* scene_registry = (IsValid(settings) && settings->_StorySceneRegistry.IsNull() == false)
		? settings->_StorySceneRegistry.LoadSynchronous()
		: nullptr;

	if (branch_node_data->GetBranchID().IsValid() == false)
	{
		node_errors.Add(TEXT("BranchID is empty."));
	}
	else if (_used_branch_ids.Contains(branch_node_data->GetBranchID().Get()))
	{
		node_errors.Add(FString::Printf(TEXT("Duplicated BranchID: %s"), *branch_node_data->GetBranchID().Get().ToString()));
	}
	else
	{
		_used_branch_ids.Add(branch_node_data->GetBranchID().Get());
	}

	if (IsInvalid(branch_node_data->GetBranchTemplate()))
	{
		node_errors.Add(TEXT("BranchTemplate is missing."));
	}

	const TMap<int32, FStorySceneBranchLink>& next_links_by_pin_index = branch_node_data->GetNextLinksByPinIndex();
	for (int32 next_pin_index = 0; next_pin_index < branch_node_data->GetBranchCount(); ++next_pin_index)
	{
		const FStorySceneBranchLink* next_link = next_links_by_pin_index.Find(next_pin_index);
		if (next_link == nullptr || next_link->IsValid() == false)
		{
			node_errors.Add(FString::Printf(TEXT("Next pin index '%d' is not connected."), next_pin_index));
		}
	}

	for (const TPair<int32, FStorySceneBranchLink>& next_link_pair : next_links_by_pin_index)
	{
		const int32 next_pin_index = next_link_pair.Key;
		const FStorySceneBranchLink& next_link = next_link_pair.Value;

		if (next_pin_index < 0 || next_pin_index >= branch_node_data->GetBranchCount())
		{
			node_errors.Add(FString::Printf(TEXT("Next pin index '%d' is out of range."), next_pin_index));
			continue;
		}

		if (next_link.IsShotLink())
		{
			if (IsInvalid(_StorySceneAsset->FindShotNode(next_link.NextShotID)))
			{
				node_errors.Add(FString::Printf(TEXT("NextShotID '%s' does not exist."), *next_link.NextShotID.Get().ToString()));
			}
			continue;
		}

		if (next_link.IsSceneLink())
		{
			if (scene_registry == nullptr)
			{
				node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is not registered in StorySceneRegistry."), *next_link.NextSceneID.Get().ToString()));
			}
			else
			{
				const int32 matching_scene_count = CountSceneRegistryMatches(scene_registry, next_link.NextSceneID);
				if (matching_scene_count == 0)
				{
					node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is not registered in StorySceneRegistry."), *next_link.NextSceneID.Get().ToString()));
				}
				else if (matching_scene_count > 1)
				{
					node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is duplicated in StorySceneRegistry."), *next_link.NextSceneID.Get().ToString()));
				}
			}
			continue;
		}

		node_errors.Add(TEXT("NextLinksByPinIndex contains an invalid branch link."));
	}

	if (node_errors.Num() == 0)
	{
		return;
	}

	const FString joined_error_message = FString::Join(node_errors, TEXT("\n"));
	_branch_node->SetCompileError(joined_error_message);
	_out_errors.Add(FString::Printf(TEXT("[%s] %s"), *branch_node_data->GetDisplayNameText().ToString(), *joined_error_message));
}

void FStorySceneEditor::ValidateShotNode(UStorySceneGraphNode_Shot* _shot_node, TSet<FName>& _used_shot_ids, TArray<FString>& _out_errors) const
{
	if (IsInvalid(_shot_node))
	{
		return;
	}

	UStorySceneNodeData* shot_node_data = _shot_node->GetShotNodeData();
	if (IsInvalid(shot_node_data))
	{
		const FString error_message = TEXT("A Shot node does not have valid node data.");
		_out_errors.Add(error_message);
		_shot_node->SetCompileError(error_message);
		return;
	}

	TArray<FString> node_errors;
	const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();

	if (shot_node_data->GetShotID().IsValid() == false)
	{
		node_errors.Add(TEXT("ShotID is empty."));
	}
	else if (_used_shot_ids.Contains(shot_node_data->GetShotID().Get()))
	{
		node_errors.Add(FString::Printf(TEXT("Duplicated ShotID: %s"), *shot_node_data->GetShotID().Get().ToString()));
	}
	else
	{
		_used_shot_ids.Add(shot_node_data->GetShotID().Get());
	}

	if (IsInvalid(shot_node_data->GetShotTemplate()))
	{
		node_errors.Add(TEXT("ShotTemplate is missing."));
	}

	if (IsValid(settings) == false || settings->_StorySceneRegistry.IsNull())
	{
		node_errors.Add(TEXT("StorySceneRegistry is missing."));
	}

	UStorySceneRegistryAsset* scene_registry = (IsValid(settings) && settings->_StorySceneRegistry.IsNull() == false)
		? settings->_StorySceneRegistry.LoadSynchronous()
		: nullptr;

	const FStorySceneBranchLink next_link = shot_node_data->GetNextLink();
	if (next_link.IsValid() == false)
	{
		if (node_errors.Num() == 0)
		{
			return;
		}
	}
	else if (next_link.IsShotLink())
	{
		if (IsInvalid(_StorySceneAsset->FindShotNode(next_link.NextShotID)))
		{
			node_errors.Add(FString::Printf(TEXT("NextShotID '%s' does not exist."), *next_link.NextShotID.Get().ToString()));
		}
	}
	else if (next_link.IsBranchLink())
	{
		if (IsInvalid(_StorySceneAsset->FindBranchNode(next_link.NextBranchID)))
		{
			node_errors.Add(FString::Printf(TEXT("NextBranchID '%s' does not exist."), *next_link.NextBranchID.Get().ToString()));
		}
	}
	else if (next_link.IsSceneLink())
	{
		if (scene_registry == nullptr)
		{
			node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is not registered in StorySceneRegistry."), *next_link.NextSceneID.Get().ToString()));
		}
		else
		{
			const int32 matching_scene_count = CountSceneRegistryMatches(scene_registry, next_link.NextSceneID);
			if (matching_scene_count == 0)
			{
				node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is not registered in StorySceneRegistry."), *next_link.NextSceneID.Get().ToString()));
			}
			else if (matching_scene_count > 1)
			{
				node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is duplicated in StorySceneRegistry."), *next_link.NextSceneID.Get().ToString()));
			}
		}
	}
	else
	{
		node_errors.Add(TEXT("NextLink contains an invalid branch link."));
	}

	if (node_errors.Num() == 0)
	{
		return;
	}

	const FString joined_error_message = FString::Join(node_errors, TEXT("\n"));
	_shot_node->SetCompileError(joined_error_message);
	_out_errors.Add(FString::Printf(TEXT("[%s] %s"), *shot_node_data->GetDisplayNameText().ToString(), *joined_error_message));
}

void FStorySceneEditor::ValidateTransitionNode(UStorySceneGraphNode_Transition* _transition_node, TArray<FString>& _out_errors) const
{
	if (IsInvalid(_transition_node))
	{
		return;
	}

	TArray<FString> node_errors;
	const UEdGraphPin* input_pin = _transition_node->FindPin(TEXT("In"));
	const bool is_connected_from_shot = input_pin && input_pin->LinkedTo.Num() > 0;
	const UStoryFlowDeveloperSettings* settings = GetDefault<UStoryFlowDeveloperSettings>();

	if (is_connected_from_shot && _transition_node->GetNextSceneID().IsValid() == false)
	{
		node_errors.Add(TEXT("NextSceneID is missing."));
	}

	if (_transition_node->GetNextSceneID().IsValid())
	{
		if (IsInvalid(settings) || settings->_StorySceneRegistry.IsNull())
		{
			node_errors.Add(TEXT("StorySceneRegistry is missing."));
		}
		else if (UStorySceneRegistryAsset* scene_registry = settings->_StorySceneRegistry.LoadSynchronous())
		{
			const int32 matching_scene_count = CountSceneRegistryMatches(scene_registry, _transition_node->GetNextSceneID());
			if (matching_scene_count == 0)
			{
				node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is not registered in StorySceneRegistry."), *_transition_node->GetNextSceneID().Get().ToString()));
			}
			else if (matching_scene_count > 1)
			{
				node_errors.Add(FString::Printf(TEXT("NextSceneID '%s' is duplicated in StorySceneRegistry."), *_transition_node->GetNextSceneID().Get().ToString()));
			}
		}
	}

	if (node_errors.Num() == 0)
	{
		return;
	}

	const FString joined_error_message = FString::Join(node_errors, TEXT("\n"));
	_transition_node->SetCompileError(joined_error_message);
	_out_errors.Add(FString::Printf(TEXT("[Transition] %s"), *joined_error_message));
}

void FStorySceneEditor::ClearNodeCompileMessages(UStorySceneEdGraph* _graph) const
{
	if (IsInvalid(_graph))
	{
		return;
	}

	if (UStorySceneGraphNode_Entry* entry_node = _graph->FindEntryNode())
	{
		entry_node->ClearCompileMessage();
	}

	for (UEdGraphNode* node : _graph->Nodes)
	{
		UStorySceneGraphNode_Branch* branch_node = Cast<UStorySceneGraphNode_Branch>(node);
		if (IsValid(branch_node))
		{
			branch_node->ClearCompileMessage();
			continue;
		}

		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsValid(shot_node))
		{
			shot_node->ClearCompileMessage();
			continue;
		}

		UStorySceneGraphNode_Transition* transition_node = Cast<UStorySceneGraphNode_Transition>(node);
		if (IsValid(transition_node))
		{
			transition_node->ClearCompileMessage();
		}
	}
}

void FStorySceneEditor::SyncGraphNodePins(UStorySceneEdGraph* _graph) const
{
	if (IsInvalid(_graph))
	{
		return;
	}

	for (UEdGraphNode* node : _graph->Nodes)
	{
		if (UStorySceneGraphNode_Branch* branch_node = Cast<UStorySceneGraphNode_Branch>(node))
		{
			branch_node->SyncNextPinsToNodeData();
		}
	}
}

void FStorySceneEditor::MarkCompileDirty()
{
	_CompileStatus = ECompileStatus::Dirty;
}

void FStorySceneEditor::DeleteSelectedNodes()
{
	if (IsInvalid(_GraphEditorWidget))
	{
		return;
	}

	UEdGraph* graph = _GraphEditorWidget->GetCurrentGraph();
	if (IsInvalid(graph))
	{
		return;
	}

	const FScopedTransaction transaction(NSLOCTEXT("StorySceneEditor", "DeleteNodes", "Delete Story Scene Nodes"));
	graph->Modify();

	const TSet<UObject*> selected_nodes = _GraphEditorWidget->GetSelectedNodes();
	for (UObject* selected_object : selected_nodes)
	{
		UEdGraphNode* selected_node = Cast<UEdGraphNode>(selected_object);
		if (IsInvalid(selected_node) || selected_node->CanUserDeleteNode() == false)
		{
			continue;
		}

		selected_node->Modify();
		selected_node->DestroyNode();
	}

	MarkCompileDirty();

	_GraphEditorWidget->ClearSelectionSet();
	_GraphEditorWidget->NotifyGraphChanged();
}

bool FStorySceneEditor::CanDeleteSelectedNodes() const
{
	return _GraphEditorWidget.IsValid() && _GraphEditorWidget->GetSelectedNodes().IsEmpty() == false;
}

void FStorySceneEditor::UndoGraphAction()
{
	if (IsValid(GEditor))
	{
		GEditor->UndoTransaction();
	}
}

void FStorySceneEditor::RedoGraphAction()
{
	if (IsValid(GEditor))
	{
		GEditor->RedoTransaction();
	}
}

bool FStorySceneEditor::CanUndoGraphAction() const
{
	return IsAllValid(GEditor, GEditor->Trans) && GEditor->Trans->CanUndo();
}

bool FStorySceneEditor::CanRedoGraphAction() const
{
	return IsAllValid(GEditor, GEditor->Trans) && GEditor->Trans->CanRedo();
}

UStorySceneEdGraph* FStorySceneEditor::GetOrCreateGraph() const
{
	check(_StorySceneAsset);

	UStorySceneEdGraph* graph = Cast<UStorySceneEdGraph>(_StorySceneAsset->GetEditorGraph());
	if (IsInvalid(graph))
	{
		graph = NewObject<UStorySceneEdGraph>(_StorySceneAsset.Get(), UStorySceneEdGraph::StaticClass(), NAME_None, RF_Transactional);
		graph->Schema = UStorySceneGraphSchema::StaticClass();
		graph->EnsureEntryNode();
		_StorySceneAsset->SetEditorGraph(graph);
	}

	return graph;
}
