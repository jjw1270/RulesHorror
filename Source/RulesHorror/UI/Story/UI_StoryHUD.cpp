// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI/Story/UI_StoryHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "CommonUtils.h"

namespace
{
	static const FLinearColor ObjectiveColor(0.92f, 0.92f, 0.86f, 1.0f);
	static const FLinearColor SpeakerColor(1.0f, 0.82f, 0.45f, 1.0f);
	static const FLinearColor LineColor(0.96f, 0.96f, 0.96f, 1.0f);
	static const FLinearColor NarrationColor(0.75f, 0.78f, 0.82f, 1.0f);
	static const FLinearColor DialogueBoxColor(0.0f, 0.0f, 0.0f, 0.72f);
	static const float DialogueBoxMinHeight = 168.0f;
	static const float DialogueBoxBottomMargin = 48.0f;
}

bool UUI_StoryHUD::Initialize()
{
	const bool is_initialized = Super::Initialize();
	if (is_initialized && IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildWidgetTree();
	}

	return is_initialized;
}

void UUI_StoryHUD::BuildWidgetTree()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanel* root_canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = root_canvas;

	// 목표 문구 (왼쪽 위)
	_ObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ObjectiveText"));
	_ObjectiveText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 22));
	_ObjectiveText->SetColorAndOpacity(FSlateColor(ObjectiveColor));
	_ObjectiveText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	_ObjectiveText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	_ObjectiveText->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* objective_slot = root_canvas->AddChildToCanvas(_ObjectiveText);
	objective_slot->SetAnchors(FAnchors(0.0f, 0.0f));
	objective_slot->SetAlignment(FVector2D(0.0f, 0.0f));
	objective_slot->SetPosition(FVector2D(48.0f, 40.0f));
	objective_slot->SetAutoSize(true);

	// 대사 상자 (아래 가운데). 가로는 화면 폭의 76%, 세로는 최소 168px이고 줄이 많으면 위로 늘어난다. 아래에서 48px 띄운다.
	USizeBox* dialogue_size_box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DialogueSizeBox"));
	dialogue_size_box->SetMinDesiredHeight(DialogueBoxMinHeight);

	UCanvasPanelSlot* dialogue_slot = root_canvas->AddChildToCanvas(dialogue_size_box);
	dialogue_slot->SetAnchors(FAnchors(0.12f, 1.0f, 0.88f, 1.0f));
	dialogue_slot->SetAlignment(FVector2D(0.5f, 1.0f));
	// AutoSize라 높이는 내용(최소 높이는 SizeBox)으로 정해지고, Offsets는 위치(아래에서 띄울 거리)만 쓴다.
	dialogue_slot->SetOffsets(FMargin(0.0f, -DialogueBoxBottomMargin, 0.0f, 0.0f));
	dialogue_slot->SetAutoSize(true);

	_DialogueBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueBox"));
	_DialogueBox->SetBrushColor(DialogueBoxColor);
	_DialogueBox->SetPadding(FMargin(36.0f, 22.0f));
	_DialogueBox->SetVisibility(ESlateVisibility::Collapsed);
	dialogue_size_box->SetContent(_DialogueBox);

	UVerticalBox* dialogue_vertical_box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueVerticalBox"));
	_DialogueBox->SetContent(dialogue_vertical_box);

	_SpeakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerText"));
	_SpeakerText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
	_SpeakerText->SetColorAndOpacity(FSlateColor(SpeakerColor));
	UVerticalBoxSlot* speaker_slot = dialogue_vertical_box->AddChildToVerticalBox(_SpeakerText);
	speaker_slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	_LineText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LineText"));
	_LineText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 26));
	_LineText->SetColorAndOpacity(FSlateColor(LineColor));
	_LineText->SetAutoWrapText(true);
	dialogue_vertical_box->AddChildToVerticalBox(_LineText);
}

void UUI_StoryHUD::SetObjective(const FText& _objective)
{
	if (IsInvalid(_ObjectiveText))
	{
		return;
	}

	if (_objective.IsEmpty())
	{
		_ObjectiveText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	_ObjectiveText->SetText(FText::Format(FText::FromString(TEXT("목표: {0}")), _objective));
	_ObjectiveText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UUI_StoryHUD::ShowDialogueLine(const FText& _speaker, const FText& _text)
{
	if (IsInvalid(_DialogueBox))
	{
		return;
	}

	const bool is_narration = _speaker.IsEmpty();
	_SpeakerText->SetText(_speaker);
	_SpeakerText->SetVisibility(is_narration ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	_LineText->SetText(_text);
	_LineText->SetColorAndOpacity(FSlateColor(is_narration ? NarrationColor : LineColor));
	_DialogueBox->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UUI_StoryHUD::HideDialogue()
{
	if (IsValid(_DialogueBox))
	{
		_DialogueBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}
