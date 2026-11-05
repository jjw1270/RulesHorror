// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Office/MonitorScreen/Explorer/UI_SiteBase.h"
#include "SaveGame/RulesHorrorProgressDefines.h"
#include "Site_IWish.generated.h"

class UTextBlock;
class UClickButton;
class UButtonBase;
class UUI_IWishSlot;

/*
 * 괴담 사이트 i-wish (www.i-wish.co.kr) 첫 화면. 기준: 기획/챕터-씬-기획서.md "괴담 사이트: i-wish", CH02_S03.
 * - 낙인 칸 세 개(UUI_IWishSlot)를 진행 상태로 칠하고, 진행 상태가 바뀌면 다시 칠한다.
 * - 열린 칸을 누르면 경고 팝업을 띄우고, 확인을 누르면 시나리오 입장 이벤트(Site_EnterScenario_0N)를 StoryFlow로 보낸다.
 *   맵 이동은 그 이벤트를 기다리던 Scene이 맡는다. 기다리는 Shot이 없으면 아무 일도 하지 않는다.
 * - 화면이 열리면 Site_Opened 이벤트를 보낸다.
 */
UCLASS(abstract)
class RULESHORROR_API USite_IWish : public UUI_SiteBase
{
	GENERATED_BODY()

public:
	USite_IWish(const FObjectInitializer& _object_initializer);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_MemberName = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUI_IWishSlot> Slot_01 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUI_IWishSlot> Slot_02 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUI_IWishSlot> Slot_03 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> OVL_Warning = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_WarningTitle = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_WarningMessage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UClickButton> BTN_WarningConfirm = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UClickButton> BTN_WarningCancel = nullptr;

protected:
	/* 회원 칸 인사. {Nickname}은 사이트 가입 때 정한 닉네임 */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _MemberNameFormat;

	/* 닉네임이 없을 때 (가입 전) */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _GuestName;

	/* 경고 팝업 아래 문구 */
	UPROPERTY(EditAnywhere, Category = "IWish")
	FText _WarningMessage;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUI_IWishSlot>> _Slots;

	TWeakObjectPtr<UUI_IWishSlot> _PendingSlot;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/* 닉네임과 낙인 칸을 지금 진행 상태로 다시 그린다. */
	UFUNCTION(BlueprintCallable, Category = "IWish")
	void RefreshSite();

	UFUNCTION(BlueprintPure, Category = "IWish")
	bool IsWarningOpen() const;

protected:
	void RefreshMemberName();
	void RefreshSlots();

	void OnSlotClicked(UUI_IWishSlot* _slot);
	void ShowWarning(UUI_IWishSlot* _slot);
	void HideWarning();

	UFUNCTION()
	void OnClickWarningConfirm(UButtonBase* _btn);

	UFUNCTION()
	void OnClickWarningCancel(UButtonBase* _btn);

	UFUNCTION()
	void OnProgressFlagChanged(ERulesHorrorProgressFlag _flag, bool _value);
};
