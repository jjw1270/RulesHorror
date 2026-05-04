// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomSaveGame.h"
#include "StoryFlowDefines.h"
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
	void SaveStoryFlowRef(const FStoryFlowRef& _value);
	const FStoryFlowRef& GetStoryFlowRef() const;

#pragma endregion Story Flow Ref
///////////////////////////////////////////////////////////////////////////////////////

};
