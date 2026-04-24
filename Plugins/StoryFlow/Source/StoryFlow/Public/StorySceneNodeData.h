// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StoryFlowDefines.h"
#include "StorySceneNodeData.generated.h"

class UStoryShotBase;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class STORYFLOW_API UStorySceneNodeData : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryFlow")
	FStoryShotID _ShotID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryFlow")
	FText _DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryFlow", meta = (MultiLine = true))
	FText _Description;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "StoryFlow")
	TObjectPtr<UStoryShotBase> _ShotTemplate = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StoryFlow")
	TArray<FStoryShotID> _NextShotIDs;

#if WITH_EDITOR
public:
	void SetShotID(const FStoryShotID& _shot_id);
	void SetNextShotIDs(const TArray<FStoryShotID>& _next_shot_ids);
#endif

public:
	UFUNCTION(BlueprintPure)
	const FStoryShotID& GetShotID() const { return _ShotID; }

	UFUNCTION(BlueprintPure)
	FText GetDisplayNameText() const;

	UFUNCTION(BlueprintPure)
	const FText& GetDescriptionText() const { return _Description; }

	UFUNCTION(BlueprintPure)
	UStoryShotBase* GetShotTemplate() const { return _ShotTemplate; }

	UFUNCTION(BlueprintPure)
	const TArray<FStoryShotID>& GetNextShotIDs() const { return _NextShotIDs; }
};
