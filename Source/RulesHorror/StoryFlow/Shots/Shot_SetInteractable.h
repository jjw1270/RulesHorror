// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "Shot_SetInteractable.generated.h"

/*
 * 태그가 붙은 레벨 오브젝트의 상호작용을 켜거나 끈다 (조사 오브젝트 StoryInspectable, 컴퓨터).
 * 바로 다음 Shot으로 넘어간다.
 */
UCLASS(meta = (DisplayName = "Set Interactable"))
class RULESHORROR_API UShot_SetInteractable : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	/* 레벨 액터의 Tags에 들어 있는 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName _ActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool _Enabled = true;

protected:
	virtual void OnEnterShot_Implementation() override;
};
