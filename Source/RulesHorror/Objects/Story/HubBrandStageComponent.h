// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "HubBrandStageComponent.generated.h"

/*
 * 꿈속 허브(밤의 사무실)가 낙인 수에 따라 바뀌게 하는 컴포넌트.
 * 허브에 놓은 액터에 붙이면, 얻은 낙인 수가 [_MinBrandCount, _MaxBrandCount] 안일 때만 그 액터를 보이고(충돌 포함), 아니면 숨긴다.
 * 낙인을 얻는 순간(진행 상태 변경 알림)에도 바로 바뀐다.
 * 예: 낙인 1개부터 보이는 핏자국(Min 1), 낙인 0개일 때만 있는 소품(Min 0, Max 0).
 */
UCLASS(ClassGroup = (Story), meta = (BlueprintSpawnableComponent))
class RULESHORROR_API UHubBrandStageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHubBrandStageComponent();

protected:
	/* 낙인 수가 이 값 이상이면 보인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub", meta = (ClampMin = "0", ClampMax = "3"))
	int32 _MinBrandCount = 1;

	/* 낙인 수가 이 값 이하일 때만 보인다. -1이면 위쪽 제한이 없다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub", meta = (ClampMin = "-1", ClampMax = "3"))
	int32 _MaxBrandCount = -1;

	/* 숨길 때 충돌도 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hub")
	bool _DisableCollisionWhenHidden = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type _end_play_reason) override;

public:
	/* 지금 낙인 수로 보임/숨김을 다시 정한다. */
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void RefreshStage();

	UFUNCTION(BlueprintPure, Category = "Hub")
	bool IsActiveForBrandCount(int32 _brand_count) const;

protected:
	UFUNCTION()
	void OnProgressFlagChanged(ERulesHorrorProgressFlag _flag, bool _value);
};
