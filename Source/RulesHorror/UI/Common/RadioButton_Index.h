// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Components/RadioButton.h"
#include "RadioButton_Index.generated.h"


UCLASS(abstract)
class RULESHORROR_API URadioButton_Index : public URadioButton
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<class UTextBlock> TXT_Index = nullptr;

protected:
	int32 _Index = -1;

public:
	void SetIndex(int32 _new_index);

	UFUNCTION(BlueprintPure)
	int32 GetIndex() const { return _Index; }
};
