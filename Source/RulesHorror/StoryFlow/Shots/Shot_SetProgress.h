// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "Shot_SetProgress.generated.h"

/*
 * 진행 플래그(낙인 획득, 시나리오 해금/클리어, 엔딩 조건)를 켜고 바로 저장한 뒤 다음 Shot으로 넘어간다.
 * 이어하기 위치까지 저장하려면 이 노드(또는 앞 노드)의 IsCheckpoint를 켠다.
 */
UCLASS(meta = (DisplayName = "Set Progress"))
class RULESHORROR_API UShot_SetProgress : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	/* 이 Shot에 들어오면 켤 진행 플래그. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progress")
	TArray<ERulesHorrorProgressFlag> _FlagsToSet;

protected:
	virtual void OnEnterShot_Implementation() override;
};
