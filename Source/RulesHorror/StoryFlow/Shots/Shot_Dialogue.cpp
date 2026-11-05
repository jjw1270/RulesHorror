// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_Dialogue.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "CommonUtils.h"

void UShot_Dialogue::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this);
	if (IsInvalid(story_subsystem) || _Lines.Num() == 0)
	{
		if (IsInvalid(story_subsystem))
		{
			TRACE_ERROR(TEXT("story subsystem invalid."));
		}

		FinishShot();
		return;
	}

	if (_WaitForFinish)
	{
		story_subsystem->PlayDialogue(_Lines, FOnStoryDialogueFinished::CreateUObject(this, &UShot_Dialogue::OnDialogueFinished));
	}
	else
	{
		story_subsystem->PlayDialogue(_Lines, FOnStoryDialogueFinished());
		FinishShot();
	}
}

void UShot_Dialogue::OnExitShot_Implementation()
{
	// 기다리던 대사가 끝나기 전에 Scene이 멈추면 대사도 같이 닫는다.
	if (_WaitForFinish && IsFinished() == false)
	{
		if (URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this))
		{
			story_subsystem->StopDialogue();
		}
	}

	Super::OnExitShot_Implementation();
}

void UShot_Dialogue::OnDialogueFinished()
{
	if (IsRunning() && IsFinished() == false)
	{
		FinishShot();
	}
}
