// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "StoryFlow/RulesHorrorStoryDefines.h"
#include "Shot_Dialogue.generated.h"

/*
 * 화면 아래 대사/자막을 한 줄씩 보여준다. 말한 사람이 비어 있으면 지문으로 보인다.
 * 각 줄은 시간이 지나거나 클릭/스페이스/엔터로 넘어간다. 대사가 나오는 동안 조사(상호작용)는 막힌다.
 */
UCLASS(meta = (DisplayName = "Dialogue"))
class RULESHORROR_API UShot_Dialogue : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FStoryDialogueLine> _Lines;

	/* 끄면 대사를 띄워 두고 바로 다음 Shot으로 넘어간다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool _WaitForFinish = true;

protected:
	virtual void OnEnterShot_Implementation() override;
	virtual void OnExitShot_Implementation() override;

	void OnDialogueFinished();
};
