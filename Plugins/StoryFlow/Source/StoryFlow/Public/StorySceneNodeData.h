// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StoryFlowDefines.h"
#include "StorySceneNodeData.generated.h"

class UStoryShotBase;

USTRUCT(BlueprintType)
struct STORYFLOW_API FStorySceneBranchLink
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StoryFlow")
	FStoryShotID NextShotID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StoryFlow")
	FStoryBranchID NextBranchID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StoryFlow")
	FStorySceneID NextSceneID;

	bool IsValid() const
	{
		return NextShotID.IsValid() || NextBranchID.IsValid() || NextSceneID.IsValid();
	}

	bool IsShotLink() const
	{
		return NextShotID.IsValid();
	}

	bool IsBranchLink() const
	{
		return NextBranchID.IsValid();
	}

	bool IsSceneLink() const
	{
		return NextSceneID.IsValid();
	}

	bool operator==(const FStorySceneBranchLink& _other) const
	{
		return NextShotID == _other.NextShotID
			&& NextBranchID == _other.NextBranchID
			&& NextSceneID == _other.NextSceneID;
	}
};

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
	TArray<FStorySceneBranchLink> _NextLinks;

#if WITH_EDITOR
public:
	void SetShotID(const FStoryShotID& _shot_id);
	void SetNextLinks(const TArray<FStorySceneBranchLink>& _next_links);
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
	const TArray<FStorySceneBranchLink>& GetNextLinks() const { return _NextLinks; }
};
