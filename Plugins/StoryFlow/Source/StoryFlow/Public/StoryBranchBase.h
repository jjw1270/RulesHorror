// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StoryFlowDefines.h"
#include "StoryBranchBase.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew)
class STORYFLOW_API UStoryBranchBase : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "StoryFlow")
	FStoryFlowRef _StartRef;

public:
	virtual UWorld* GetWorld() const override;

public:
	void InitializeBranch(const FStoryFlowRef& _story_flow_ref);

	UFUNCTION(BlueprintNativeEvent, Category = "StoryFlow")
	int32 SelectNextIndex(int32 _next_count) const;
	virtual int32 SelectNextIndex_Implementation(int32 _next_count) const { return 0; }

	UFUNCTION(BlueprintPure, Category = "StoryFlow")
	const FStoryFlowRef& GetStartRef() const { return _StartRef; }
};
