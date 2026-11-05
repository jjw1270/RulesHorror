// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WidgetBase.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "UI_IWishSlot.generated.h"

class UBorder;
class UTextBlock;
class UClickButton;
class UButtonBase;
class URulesHorrorProgressSubsystem;

/* i-wish 낙인 칸 상태. 진행 상태(시나리오 해금/클리어)에서 정한다. */
UENUM(BlueprintType)
enum class EIWishSlotState : uint8
{
	Locked		UMETA(ToolTip = "아직 열리지 않음 (Scenario_0N_Unlocked 꺼짐)"),
	Open		UMETA(ToolTip = "들어갈 수 있음"),
	Cleared		UMETA(ToolTip = "클리어함 (Scenario_0N_Clear). 낙인을 얻었으면 낙인 이름도 보인다"),
};

/* 칸 상태별 색과 글자. */
USTRUCT(BlueprintType)
struct RULESHORROR_API FIWishSlotStyle
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor FrameColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor CardColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor LabelColor = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor TitleColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor StateColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText StateText;
};

/*
 * i-wish 사이트의 낙인 칸 하나 (Site_IWish 안에 세 개).
 * 잠김: 제목을 가리고 누를 수 없음 / 열림: 누르면 Site_IWish가 경고 팝업을 띄움 / 클리어: 누를 수 없고, 낙인을 얻었으면 낙인 이름을 보여 줌.
 */
UCLASS(abstract)
class RULESHORROR_API UUI_IWishSlot : public UWidgetBase
{
	GENERATED_BODY()

public:
	UUI_IWishSlot(const FObjectInitializer& _object_initializer);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UClickButton> BTN_Slot = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UBorder> BD_Card = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UBorder> BD_CardInner = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_SlotLabel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Title = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Brand = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_State = nullptr;

protected:
	UPROPERTY(EditAnywhere, Category = "IWish")
	ERulesHorrorScenario _Scenario = ERulesHorrorScenario::None;

	/* 칸 위 작은 글자 (예: "첫 번째 괴담") */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _SlotLabel;

	/* 열린 뒤에 보이는 괴담 이름 (예: "PC방") */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _ScenarioTitle;

	/* 잠겼을 때 괴담 이름 대신 보이는 글자 */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _LockedTitle;

	/* 낙인을 얻은 뒤 보이는 낙인 이름 (예: "억압의 낙인") */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _BrandName;

	/* 낙인을 얻은 뒤 상태 글자 */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _BrandAcquiredText;

	/* 칸을 누르면 경고 팝업에 크게 보이는 문구 (예: "약한 자의 입을 막은 자") */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _WarningTitle;

	UPROPERTY(EditAnywhere, Category = "IWish|Style")
	FIWishSlotStyle _LockedStyle;

	UPROPERTY(EditAnywhere, Category = "IWish|Style")
	FIWishSlotStyle _OpenStyle;

	UPROPERTY(EditAnywhere, Category = "IWish|Style")
	FIWishSlotStyle _ClearedStyle;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "IWish")
	EIWishSlotState _State = EIWishSlotState::Locked;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "IWish")
	bool _HasBrand = false;

public:
	DECLARE_DELEGATE_OneParam(FD_OnSlotClicked, UUI_IWishSlot*);
	FD_OnSlotClicked _OnSlotClickedEvent;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;

public:
	/* 진행 상태를 읽어 칸 모양을 바꾼다. _progress가 없으면 잠김으로 보인다. */
	void Refresh(const URulesHorrorProgressSubsystem* _progress);

	ERulesHorrorScenario GetScenario() const { return _Scenario; }
	EIWishSlotState GetState() const { return _State; }
	const FText& GetWarningTitle() const { return _WarningTitle; }

protected:
	void ApplyState(EIWishSlotState _state, bool _has_brand);

	UFUNCTION()
	void OnClickSlotButton(UButtonBase* _btn);
};
