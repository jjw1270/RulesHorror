// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI/Office/MonitorScreen/Explorer/IWishSite/UI_IWishSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Widgets/Components/ClickButton.h"
#include "SaveGame/RulesHorrorProgressSubsystem.h"
#include "CommonUtils.h"

namespace
{
	FLinearColor Hex(uint8 _r, uint8 _g, uint8 _b)
	{
		return FLinearColor(FColor(_r, _g, _b));
	}

	FIWishSlotStyle MakeStyle(const FLinearColor& _frame, const FLinearColor& _card, const FLinearColor& _label, const FLinearColor& _title, const FLinearColor& _state, const FText& _state_text)
	{
		FIWishSlotStyle style;
		style.FrameColor = _frame;
		style.CardColor = _card;
		style.LabelColor = _label;
		style.TitleColor = _title;
		style.StateColor = _state;
		style.StateText = _state_text;
		return style;
	}
}

UUI_IWishSlot::UUI_IWishSlot(const FObjectInitializer& _object_initializer)
	: Super(_object_initializer)
{
	_LockedTitle = NSLOCTEXT("IWish", "Slot_LockedTitle", "? ? ?");
	_BrandAcquiredText = NSLOCTEXT("IWish", "Slot_BrandAcquired", "낙인 획득");

	// 낙인 도장판의 도장 칸 (밝은 종이 바탕): 잠김 = 흐린 회색 도장 / 열림 = 빨간 테두리 도장 / 클리어 = 빨갛게 찍힌 도장
	_LockedStyle = MakeStyle(Hex(0xB6, 0xAD, 0x9B), Hex(0xEC, 0xE6, 0xD8), Hex(0x68, 0x5E, 0x4E), Hex(0x68, 0x5E, 0x4E), Hex(0x68, 0x5E, 0x4E),
		NSLOCTEXT("IWish", "Slot_Locked", "잠김"));
	_OpenStyle = MakeStyle(Hex(0x9C, 0x14, 0x19), Hex(0xF8, 0xF4, 0xEB), Hex(0x3D, 0x34, 0x2B), Hex(0x15, 0x11, 0x0D), Hex(0x9C, 0x14, 0x19),
		NSLOCTEXT("IWish", "Slot_Open", "[ 입장하기 ]"));
	_ClearedStyle = MakeStyle(Hex(0x5C, 0x0D, 0x11), Hex(0x9C, 0x14, 0x19), Hex(0x3D, 0x34, 0x2B), Hex(0x3D, 0x34, 0x2B), Hex(0x5C, 0x0D, 0x11),
		NSLOCTEXT("IWish", "Slot_Cleared", "괴담 클리어"));
}

void UUI_IWishSlot::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(BTN_Slot))
	{
		BTN_Slot->_OnClicked.AddDynamic(this, &UUI_IWishSlot::OnClickSlotButton);
	}
}

void UUI_IWishSlot::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 디자이너에서도 잠긴 칸으로 보이게 한다. 게임에서는 Site_IWish가 진행 상태로 다시 칠한다.
	ApplyState(_State, _HasBrand);
}

void UUI_IWishSlot::Refresh(const URulesHorrorProgressSubsystem* _progress)
{
	EIWishSlotState state = EIWishSlotState::Locked;
	bool has_brand = false;

	if (IsValid(_progress))
	{
		if (_progress->IsScenarioCleared(_Scenario))
		{
			state = EIWishSlotState::Cleared;
		}
		else if (_progress->IsScenarioUnlocked(_Scenario))
		{
			state = EIWishSlotState::Open;
		}

		has_brand = _progress->HasBrand(_Scenario);
	}

	ApplyState(state, has_brand);
}

void UUI_IWishSlot::ApplyState(EIWishSlotState _state, bool _has_brand)
{
	_State = _state;
	_HasBrand = _has_brand;

	const FIWishSlotStyle& style = (_state == EIWishSlotState::Open) ? _OpenStyle : (_state == EIWishSlotState::Cleared) ? _ClearedStyle : _LockedStyle;

	if (IsValid(BD_Card))
	{
		BD_Card->SetBrushColor(style.FrameColor);
	}

	if (IsValid(BD_CardInner))
	{
		BD_CardInner->SetBrushColor(style.CardColor);
	}

	if (IsValid(TXT_SlotLabel))
	{
		TXT_SlotLabel->SetText(_SlotLabel);
		TXT_SlotLabel->SetColorAndOpacity(FSlateColor(style.LabelColor));
	}

	if (IsValid(TXT_Title))
	{
		TXT_Title->SetText(_state == EIWishSlotState::Locked ? _LockedTitle : _ScenarioTitle);
		TXT_Title->SetColorAndOpacity(FSlateColor(style.TitleColor));
	}

	// 낙인은 클리어 뒤 따로 얻는다 (CH02_S04 클리어 -> CH02_S05 낙인). 얻은 뒤에만 낙인 이름을 보인다.
	const bool show_brand = (_state == EIWishSlotState::Cleared) && _has_brand;

	if (IsValid(TXT_Brand))
	{
		TXT_Brand->SetText(_BrandName);
		TXT_Brand->SetVisibility(show_brand ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (IsValid(TXT_State))
	{
		TXT_State->SetText(show_brand ? _BrandAcquiredText : style.StateText);
		TXT_State->SetColorAndOpacity(FSlateColor(style.StateColor));
	}

	if (IsValid(BTN_Slot))
	{
		BTN_Slot->SetButtonDisabled(_state != EIWishSlotState::Open);
	}
}

void UUI_IWishSlot::OnClickSlotButton(UButtonBase* _btn)
{
	if (_State != EIWishSlotState::Open)
		return;

	_OnSlotClickedEvent.ExecuteIfBound(this);
}
