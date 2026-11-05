// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RulesHorrorGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API URulesHorrorGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Shutdown() override;
	
public:
	static constexpr int32 SaveGameSlotIndex = 0;

	UFUNCTION(BlueprintCallable)
	void QuitGame();

	UFUNCTION(BlueprintCallable)
	void PauseGame(bool _is_pause);

	UFUNCTION(BlueprintPure)
	bool CanStartSavedGame() const;

	/* 저장 슬롯을 비우고 시작 Scene부터 시작한다. 진행 플래그도 모두 초기화된다. */
	UFUNCTION(BlueprintCallable)
	void StartNewGame();

	/* 디스크의 저장 슬롯을 다시 읽어 마지막 체크포인트부터 시작한다. 이어하기와 체크포인트 재시작에 같이 쓴다. */
	UFUNCTION(BlueprintCallable)
	void StartSavedGame();
	
};
