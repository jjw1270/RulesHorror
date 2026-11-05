// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_SetInteractable.h"
#include "Objects/Interactable/Computer.h"
#include "Objects/Story/StoryInspectable.h"
#include "Kismet/GameplayStatics.h"
#include "CommonUtils.h"

void UShot_SetInteractable::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	TArray<AActor*> tagged_actors;
	UGameplayStatics::GetAllActorsWithTag(this, _ActorTag, tagged_actors);

	int32 changed_count = 0;
	for (AActor* actor : tagged_actors)
	{
		if (AStoryInspectable* inspectable = Cast<AStoryInspectable>(actor))
		{
			inspectable->SetInspectEnabled(_Enabled);
			++changed_count;
		}
		else if (AComputer* computer = Cast<AComputer>(actor))
		{
			computer->SetInteractionEnabled(_Enabled);
			++changed_count;
		}
	}

	if (changed_count == 0)
	{
		TRACE_WARNING(TEXT("No interactable actor with tag '%s'."), *_ActorTag.ToString());
	}

	FinishShot();
}
