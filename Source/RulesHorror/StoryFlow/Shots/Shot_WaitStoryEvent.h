// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "Shot_WaitStoryEvent.generated.h"

/*
 * 스토리 이벤트가 모두 들어올 때까지 기다린다 (상호작용 대기).
 * 이벤트는 조사 오브젝트(StoryInspectable), 트리거(StoryTriggerVolume), 컴퓨터, 사이트 UI가 이름으로 보낸다.
 * 이 Shot에 들어오기 전에 이미 받은 이벤트도 기본으로 친다(같은 레벨 안에서만).
 */
UCLASS(meta = (DisplayName = "Wait Story Event"))
class RULESHORROR_API UShot_WaitStoryEvent : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	/* 모두 받아야 다음으로 넘어간다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wait")
	TArray<FName> _RequiredEvents;

	/* 켜면 이 Shot에 들어온 뒤에 받은 이벤트만 친다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wait")
	bool _IgnoreEarlierEvents = false;

	/* 이 진행 플래그가 모두 켜져 있으면 기다리지 않고 바로 넘어간다 (예: 이미 가입했으면 가입 대기 생략). 비어 있으면 늘 기다린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wait")
	TArray<ERulesHorrorProgressFlag> _SkipIfProgressFlags;

	FDelegateHandle _StoryEventHandle;
	bool _IsAwaiting = false;

protected:
	virtual void OnEnterShot_Implementation() override;
	virtual void OnExitShot_Implementation() override;

	void OnStoryEvent(FName _event_name);
	void FinishIfAllReceived();
	void StopAwaiting();
};
