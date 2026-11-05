// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryBranchBase.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "Branch_CheckProgress.generated.h"

/*
 * 진행 플래그 조건 분기. _RequiredFlags가 모두 켜져 있으면 두 번째 출력(조건 충족), 아니면 첫 번째 출력(미충족).
 * 예: 병실 엔딩 진입 = EndingUnlocked, 추격 파트 진입 = Brand_01~03.
 */
UCLASS(meta = (DisplayName = "Check Progress"))
class RULESHORROR_API UBranch_CheckProgress : public UStoryBranchBase
{
	GENERATED_BODY()

public:
	UBranch_CheckProgress();

protected:
	/* 모두 켜져 있어야 조건 충족. 비어 있으면 항상 충족. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progress")
	TArray<ERulesHorrorProgressFlag> _RequiredFlags;

public:
	virtual int32 SelectNextIndex_Implementation(int32 _next_count) const override;
};
