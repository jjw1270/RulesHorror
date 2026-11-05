// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_ComputerAction.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "Objects/Interactable/Computer.h"
#include "GameFramework/Pawn/OfficePawn.h"
#include "Kismet/GameplayStatics.h"
#include "CommonUtils.h"

void UShot_ComputerAction::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	AComputer* computer = FindComputer();
	if (IsInvalid(computer))
	{
		TRACE_ERROR(TEXT("Computer not found (tag: %s)."), *_ComputerTag.ToString());
		FinishShot();
		return;
	}

	switch (_Action)
	{
	case EStoryComputerAction::PowerOn:
		computer->SetPower(true, true);
		FinishShot();
		return;

	case EStoryComputerAction::PowerOff:
		computer->SetPower(false, true);
		FinishShot();
		return;

	case EStoryComputerAction::Use:
		break;
	}

	AOfficePawn* office_pawn = Cast<AOfficePawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (IsInvalid(office_pawn))
	{
		TRACE_ERROR(TEXT("Player pawn is not an OfficePawn."));
		FinishShot();
		return;
	}

	if (office_pawn->IsOnInteracintingComputer())
	{
		FinishShot();
		return;
	}

	URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this);
	if (IsValid(story_subsystem))
	{
		_StoryEventHandle = story_subsystem->_OnStoryEventNative.AddUObject(this, &UShot_ComputerAction::OnStoryEvent);
	}

	computer->SetInteractionEnabled(true);
	IInteractableInterface::Execute_Interact(computer, office_pawn);

	if (IsInvalid(story_subsystem))
	{
		FinishShot();
	}
}

void UShot_ComputerAction::OnExitShot_Implementation()
{
	StopListening();
	Super::OnExitShot_Implementation();
}

AComputer* UShot_ComputerAction::FindComputer() const
{
	TArray<AActor*> actors;
	if (_ComputerTag.IsNone())
	{
		UGameplayStatics::GetAllActorsOfClass(this, AComputer::StaticClass(), actors);
	}
	else
	{
		UGameplayStatics::GetAllActorsOfClassWithTag(this, AComputer::StaticClass(), _ComputerTag, actors);
	}

	return actors.Num() > 0 ? Cast<AComputer>(actors[0]) : nullptr;
}

void UShot_ComputerAction::OnStoryEvent(FName _event_name)
{
	if (_event_name != RulesHorrorStoryEvents::ComputerOpened || IsFinished())
	{
		return;
	}

	StopListening();
	FinishShot();
}

void UShot_ComputerAction::StopListening()
{
	if (_StoryEventHandle.IsValid() == false)
	{
		return;
	}

	if (URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this))
	{
		story_subsystem->_OnStoryEventNative.Remove(_StoryEventHandle);
	}

	_StoryEventHandle.Reset();
}
