// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_WaitStoryEvent.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "CommonUtils.h"

void UShot_WaitStoryEvent::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this);
	if (IsInvalid(story_subsystem))
	{
		TRACE_ERROR(TEXT("story subsystem invalid. Skipping wait."));
		FinishShot();
		return;
	}

	if (_IgnoreEarlierEvents)
	{
		story_subsystem->ForgetStoryEvents(_RequiredEvents);
	}

	story_subsystem->AddAwaitedEvents(_RequiredEvents);
	_StoryEventHandle = story_subsystem->_OnStoryEventNative.AddUObject(this, &UShot_WaitStoryEvent::OnStoryEvent);
	_IsAwaiting = true;

	FinishIfAllReceived();
}

void UShot_WaitStoryEvent::OnExitShot_Implementation()
{
	StopAwaiting();
	Super::OnExitShot_Implementation();
}

void UShot_WaitStoryEvent::OnStoryEvent(FName _event_name)
{
	if (_RequiredEvents.Contains(_event_name))
	{
		FinishIfAllReceived();
	}
}

void UShot_WaitStoryEvent::FinishIfAllReceived()
{
	if (_IsAwaiting == false || IsFinished())
	{
		return;
	}

	URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this);
	if (IsInvalid(story_subsystem))
	{
		return;
	}

	for (const FName& event_name : _RequiredEvents)
	{
		if (story_subsystem->HasReceivedStoryEvent(event_name) == false)
		{
			return;
		}
	}

	StopAwaiting();
	FinishShot();
}

void UShot_WaitStoryEvent::StopAwaiting()
{
	if (_IsAwaiting == false)
	{
		return;
	}

	_IsAwaiting = false;
	if (URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this))
	{
		story_subsystem->_OnStoryEventNative.Remove(_StoryEventHandle);
		story_subsystem->RemoveAwaitedEvents(_RequiredEvents);
	}

	_StoryEventHandle.Reset();
}
