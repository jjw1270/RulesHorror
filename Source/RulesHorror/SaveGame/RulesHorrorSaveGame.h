// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomSaveGame.h"
#include "StoryFlowDefines.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "RulesHorrorSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API URulesHorrorSaveGame : public UCustomSaveGame
{
	GENERATED_BODY()

public:
	virtual void ClearData() override;
	virtual bool IsEmpty() const;
	
#pragma region Story Flow Ref
protected:
	UPROPERTY()
	FStoryFlowRef _StoryFlowRef;

public:
	/* 이어하기 위치. SceneID가 CH##_ 형식이면 CurrentChapter도 같이 갱신한다. */
	void SaveStoryFlowRef(const FStoryFlowRef& _value);
	const FStoryFlowRef& GetStoryFlowRef() const;

#pragma endregion Story Flow Ref
///////////////////////////////////////////////////////////////////////////////////////
#pragma region Progress
public:
	/* 진행 플래그는 부모의 Bool 맵에 ERulesHorrorProgressFlag 이름을 키로 저장한다. */
	bool SaveProgressFlag(ERulesHorrorProgressFlag _flag, bool _value);
	bool GetProgressFlag(ERulesHorrorProgressFlag _flag) const;

	/* 마지막 체크포인트의 챕터 번호(CH02 -> 2). 아직 없으면 0. */
	int32 GetCurrentChapter() const;

	static const FName CurrentChapterKey;

#pragma endregion Progress
///////////////////////////////////////////////////////////////////////////////////////

};
