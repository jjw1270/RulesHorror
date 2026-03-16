// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemID.h"
#include "ItemTableRow.generated.h"

USTRUCT(BlueprintType)
struct ITEMCORE_API FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FItemID ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

#if WITH_EDITORONLY_DATA
private:
	TOptional<FItemID> CachedItemID;
#endif

public:
#if WITH_EDITOR
	virtual void OnDataTableChanged(const UDataTable* _in_data_table, const FName _in_row_name) override;
	virtual void OnPostDataImport(const UDataTable* _in_data_table, const FName _in_row_name, TArray<FString>& _out_collected_import_problems) override;

protected:
	void HandleItemIDChanged(const UDataTable* _in_data_table);
#endif
};
