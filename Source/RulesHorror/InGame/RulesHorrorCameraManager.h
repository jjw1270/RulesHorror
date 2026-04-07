// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "RulesHorrorCameraManager.generated.h"

/**
 *  Limits min/max look pitch.
 */
UCLASS()
class ARulesHorrorCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	ARulesHorrorCameraManager();
};
