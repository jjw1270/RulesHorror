// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryShotBase.h"

UWorld* UStoryShotBase::GetWorld() const
{
	const UObject* outer_object = GetOuter();
	return outer_object ? outer_object->GetWorld() : nullptr;
}

void UStoryShotBase::InitializeShot(const FStoryShotID& _shot_id)
{
	_ShotID = _shot_id;
	_IsRunning = false;
	_IsFinished = false;
	_ElapsedTime = 0.0f;
}

void UStoryShotBase::EnterShot()
{
	if (_IsRunning)
	{
		return;
	}

	_IsRunning = true;
	_IsFinished = false;
	_ElapsedTime = 0.0f;

	OnEnterShot();
}

void UStoryShotBase::TickShot(float _delta_time)
{
	if (_IsRunning == false || _IsFinished)
	{
		return;
	}

	_ElapsedTime += _delta_time;
	OnTickShot(_delta_time);
}

void UStoryShotBase::ExitShot()
{
	if (_IsRunning == false)
	{
		return;
	}

	OnExitShot();
	_IsRunning = false;
}

void UStoryShotBase::FinishShot()
{
	_IsFinished = true;
}
