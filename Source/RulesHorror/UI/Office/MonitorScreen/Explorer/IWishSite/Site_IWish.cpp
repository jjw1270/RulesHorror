// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI/Office/MonitorScreen/Explorer/IWishSite/Site_IWish.h"
#include "UI/Office/MonitorScreen/Explorer/IWishSite/UI_IWishSlot.h"
#include "Components/TextBlock.h"
#include "Widgets/Components/ClickButton.h"
#include "SaveGameSubsystem.h"
#include "SaveGame/RulesHorrorProgressSubsystem.h"
#include "StoryFlow/RulesHorrorStoryDefines.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "RulesHorrorUtils.h"

USite_IWish::USite_IWish(const FObjectInitializer& _object_initializer)
	: Super(_object_initializer)
{
	_MemberNameFormat = NSLOCTEXT("IWish", "MemberNameFormat", "{Nickname}님, 환영합니다.");
	_GuestName = NSLOCTEXT("IWish", "GuestName", "방문자");
	_WarningMessage = NSLOCTEXT("IWish", "WarningMessage", "입장하시겠습니까?");
}

void USite_IWish::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	_Slots.Reset();
	for (UUI_IWishSlot* slot : { Slot_01.Get(), Slot_02.Get(), Slot_03.Get() })
	{
		if (IsValid(slot))
		{
			slot->_OnSlotClickedEvent.BindUObject(this, &USite_IWish::OnSlotClicked);
			_Slots.Add(slot);
		}
	}

	if (IsValid(BTN_WarningConfirm))
	{
		BTN_WarningConfirm->_OnClicked.AddDynamic(this, &USite_IWish::OnClickWarningConfirm);
	}

	if (IsValid(BTN_WarningCancel))
	{
		BTN_WarningCancel->_OnClicked.AddDynamic(this, &USite_IWish::OnClickWarningCancel);
	}
}

void USite_IWish::NativeConstruct()
{
	Super::NativeConstruct();

	if (URulesHorrorProgressSubsystem* progress = URulesHorrorUtils::GetGameInstanceSubsystem<URulesHorrorProgressSubsystem>(this))
	{
		progress->_OnProgressFlagChanged.AddUniqueDynamic(this, &USite_IWish::OnProgressFlagChanged);
	}

	HideWarning();
	RefreshSite();

	URulesHorrorStorySubsystem::Send(this, RulesHorrorStoryEvents::SiteOpened);
}

void USite_IWish::NativeDestruct()
{
	if (URulesHorrorProgressSubsystem* progress = URulesHorrorUtils::GetGameInstanceSubsystem<URulesHorrorProgressSubsystem>(this))
	{
		progress->_OnProgressFlagChanged.RemoveDynamic(this, &USite_IWish::OnProgressFlagChanged);
	}

	Super::NativeDestruct();
}

void USite_IWish::RefreshSite()
{
	RefreshMemberName();
	RefreshSlots();
}

bool USite_IWish::IsWarningOpen() const
{
	return IsValid(OVL_Warning) && OVL_Warning->IsVisible();
}

void USite_IWish::RefreshMemberName()
{
	if (IsInvalid(TXT_MemberName))
		return;

	FString nickname;
	if (const auto save_game = USaveGameHelper::GetCurrentSaveGame_ReadOnly(this))
	{
		save_game->FindSavedStringData(TEXT("Nickname"), nickname);
	}

	FFormatNamedArguments args;
	args.Add(TEXT("Nickname"), nickname.IsEmpty() ? _GuestName : FText::FromString(nickname));
	TXT_MemberName->SetText(FText::Format(_MemberNameFormat, args));
}

void USite_IWish::RefreshSlots()
{
	const URulesHorrorProgressSubsystem* progress = URulesHorrorUtils::GetGameInstanceSubsystem<URulesHorrorProgressSubsystem>(this);

	for (UUI_IWishSlot* slot : _Slots)
	{
		if (IsValid(slot))
		{
			slot->Refresh(progress);
		}
	}

	// 열어 둔 경고 팝업의 칸이 더는 열린 칸이 아니면 닫는다.
	if (_PendingSlot.IsValid() && _PendingSlot->GetState() != EIWishSlotState::Open)
	{
		HideWarning();
	}
}

void USite_IWish::OnSlotClicked(UUI_IWishSlot* _slot)
{
	if (IsInvalid(_slot) || _slot->GetState() != EIWishSlotState::Open || IsWarningOpen())
		return;

	ShowWarning(_slot);
}

void USite_IWish::ShowWarning(UUI_IWishSlot* _slot)
{
	_PendingSlot = _slot;

	if (IsValid(TXT_WarningTitle))
	{
		TXT_WarningTitle->SetText(_slot->GetWarningTitle());
	}

	if (IsValid(TXT_WarningMessage))
	{
		TXT_WarningMessage->SetText(_WarningMessage);
	}

	if (IsValid(OVL_Warning))
	{
		OVL_Warning->SetVisibility(ESlateVisibility::Visible);
	}
}

void USite_IWish::HideWarning()
{
	_PendingSlot.Reset();

	if (IsValid(OVL_Warning))
	{
		OVL_Warning->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USite_IWish::OnClickWarningConfirm(UButtonBase* _btn)
{
	const ERulesHorrorScenario scenario = _PendingSlot.IsValid() ? _PendingSlot->GetScenario() : ERulesHorrorScenario::None;
	HideWarning();

	const FName event_name = RulesHorrorStoryEvents::GetSiteEnterScenarioEvent(scenario);
	if (event_name.IsNone())
		return;

	// 시나리오 진입(레벨 이동)은 이 이벤트를 기다리는 Scene이 맡는다. 사이트가 맵을 직접 열지 않는다.
	if (URulesHorrorStorySubsystem::Send(this, event_name) == false)
	{
		TRACE_LOG(TEXT("No story shot is waiting for %s."), *event_name.ToString());
	}
}

void USite_IWish::OnClickWarningCancel(UButtonBase* _btn)
{
	HideWarning();
}

void USite_IWish::OnProgressFlagChanged(ERulesHorrorProgressFlag _flag, bool _value)
{
	RefreshSlots();
}
