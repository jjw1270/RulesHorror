// Copyright (c) 2026 장윤제. All rights reserved.

#include "StorySceneEditor.h"
#include "StorySceneAsset.h"
#include "StorySceneNodeData.h"
#include "Graph/StorySceneEdGraph.h"
#include "Graph/StorySceneGraphNode_Entry.h"
#include "Graph/StorySceneGraphSchema.h"
#include "Graph/StorySceneGraphNode_Shot.h"
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
#include "Editor/Transactor.h"
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
}

FStorySceneEditor::~FStorySceneEditor()
{
	_ActiveEditors.Remove(this);

	if (_ObservedGraph.IsValid() && _OnGraphChangedHandle.IsValid())
	{
		_ObservedGraph->RemoveOnGraphChangedHandler(_OnGraphChangedHandle);
	}

	if (_PreBeginPIEHandle.IsValid())
	{
		FEditorDelegates::PreBeginPIE.Remove(_PreBeginPIEHandle);
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

	if (_PreBeginPIEHandle.IsValid() == false)
	{
		_PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddRaw(this, &FStorySceneEditor::OnPreBeginPIE);
	}

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
	}

	_DetailsView->SetObject(_StorySceneAsset.Get());
}

void FStorySceneEditor::OnDetailsFinishedChangingProperties(const FPropertyChangedEvent& _property_changed_event)
{
	if (_IsCompiling)
	{
		return;
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

void FStorySceneEditor::OnPreBeginPIE(bool _is_simulating)
{
}

void FStorySceneEditor::BindGraphCommands()
{
	_GraphEditorCommands = MakeShared<FUICommandList>();

	_GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FStorySceneEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FStorySceneEditor::CanDeleteSelectedNodes));

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

	if (_StorySceneAsset->GetSceneID().IsValid() == false)
	{
		_out_errors.Add(TEXT("SceneID is empty."));
	}

	if (_StorySceneAsset->GetEntryShotID().IsValid() == false)
	{
		_out_errors.Add(TEXT("Entry node is not connected to a Shot node."));

		if (UStorySceneGraphNode_Entry* entry_node = _graph->FindEntryNode())
		{
			entry_node->SetCompileError(TEXT("Entry node is not connected to a Shot node."));
		}
	}

	TSet<FName> used_shot_ids;

	for (UEdGraphNode* node : _graph->Nodes)
	{
		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsInvalid(shot_node))
		{
			continue;
		}

		UStorySceneNodeData* shot_node_data = shot_node->GetShotNodeData();
		if (IsInvalid(shot_node_data))
		{
			const FString error_message = TEXT("A Shot node does not have valid node data.");
			_out_errors.Add(error_message);
			shot_node->SetCompileError(error_message);
			continue;
		}

		TArray<FString> node_errors;

		if (shot_node_data->GetShotID().IsValid() == false)
		{
			node_errors.Add(TEXT("ShotID is empty."));
		}
		else if (used_shot_ids.Contains(shot_node_data->GetShotID().Get()))
		{
			node_errors.Add(FString::Printf(TEXT("Duplicated ShotID: %s"), *shot_node_data->GetShotID().Get().ToString()));
		}
		else
		{
			used_shot_ids.Add(shot_node_data->GetShotID().Get());
		}

		if (IsInvalid(shot_node_data->GetShotTemplate()))
		{
			node_errors.Add(TEXT("ShotTemplate is missing."));
		}

		if (node_errors.Num() > 0)
		{
			const FString joined_error_message = FString::Join(node_errors, TEXT("\n"));
			shot_node->SetCompileError(joined_error_message);
			_out_errors.Add(FString::Printf(TEXT("[%s] %s"), *shot_node_data->GetDisplayNameText().ToString(), *joined_error_message));
		}
	}

	return _out_errors.Num() == 0;
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
		UStorySceneGraphNode_Shot* shot_node = Cast<UStorySceneGraphNode_Shot>(node);
		if (IsValid(shot_node))
		{
			shot_node->ClearCompileMessage();
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
