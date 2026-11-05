// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryShotBase.h"
#include "Shot_ComputerAction.generated.h"

class AComputer;

UENUM(BlueprintType)
enum class EStoryComputerAction : uint8
{
	PowerOn		UMETA(DisplayName = "전원 켜기"),
	PowerOff	UMETA(DisplayName = "전원 끄기"),
	/* 플레이어를 컴퓨터 앞으로 보내고 화면을 쓸 수 있을 때까지(Computer_Opened) 기다린다. 이미 컴퓨터를 쓰는 중(앞으로 가는 중 포함)이면 바로 넘어간다. */
	Use			UMETA(DisplayName = "컴퓨터 사용"),
};

/* 사무실 컴퓨터를 켜거나 끄거나, 플레이어가 컴퓨터를 쓰게 한다. */
UCLASS(meta = (DisplayName = "Computer Action"))
class RULESHORROR_API UShot_ComputerAction : public UStoryShotBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Computer")
	EStoryComputerAction _Action = EStoryComputerAction::PowerOn;

	/* 비우면 레벨의 첫 번째 컴퓨터 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Computer")
	FName _ComputerTag;

	FDelegateHandle _StoryEventHandle;

protected:
	virtual void OnEnterShot_Implementation() override;
	virtual void OnExitShot_Implementation() override;

	AComputer* FindComputer() const;
	void OnStoryEvent(FName _event_name);
	void StopListening();
};
