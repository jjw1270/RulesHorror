// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionPawnMovePoint.generated.h"

UCLASS()
class RULESHORROR_API AInteractionPawnMovePoint : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	FName _PointName;

	UPROPERTY(EditAnywhere, meta = (Tooltip = "카메라를 고정시킬것인지."))
	bool _UseFixedCamera = true;

	UPROPERTY(EditAnywhere, meta = (Tooltip = "상호작용을 활성화 할것인지"))
	bool _InteractionEnabled = true;
	
public:	
	AInteractionPawnMovePoint();

public:
	UFUNCTION(BlueprintPure)
	FName GetPointName() const { return _PointName; }

	UFUNCTION(BlueprintPure)
	bool GetUseFixedCamera() const { return _UseFixedCamera; }

	UFUNCTION(BlueprintPure)
	bool GetInteractionEnabled() const { return _InteractionEnabled; }

};
