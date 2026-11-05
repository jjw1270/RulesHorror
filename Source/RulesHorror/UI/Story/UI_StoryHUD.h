// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_StoryHUD.generated.h"

class UBorder;
class UTextBlock;

/*
 * 스토리 진행용 최소 HUD (2단계 더미): 왼쪽 위 목표 문구 + 화면 아래 대사/자막 상자.
 * 위젯 블루프린트 없이 C++에서 트리를 만든다. 클릭은 통과시킨다(HitTestInvisible).
 */
UCLASS()
class RULESHORROR_API UUI_StoryHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> _ObjectiveText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> _DialogueBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> _SpeakerText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> _LineText = nullptr;

public:
	virtual bool Initialize() override;

	void SetObjective(const FText& _objective);
	void ShowDialogueLine(const FText& _speaker, const FText& _text);
	void HideDialogue();

protected:
	void BuildWidgetTree();
};
