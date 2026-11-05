// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_Wait.h"

void UShot_Wait::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	if (_Seconds <= 0.0f)
	{
		FinishShot();
	}
}

void UShot_Wait::OnTickShot_Implementation(float _delta_time)
{
	Super::OnTickShot_Implementation(_delta_time);

	if (GetElapsedTime() >= _Seconds)
	{
		FinishShot();
	}
}
