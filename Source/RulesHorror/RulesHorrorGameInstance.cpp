// Fill out your copyright notice in the Description page of Project Settings.


#include "RulesHorrorGameInstance.h"
#include "RulesHorrorUtils.h"
#include "Kismet/KismetSystemLibrary.h"


void URulesHorrorGameInstance::QuitGame()
{
#if UE_BUILD_SHIPPING
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsValid(pc))
	{
		UKismetSystemLibrary::QuitGame(this, pc, EQuitPreference::Quit, false);
	}
#else
	if (GIsEditor && GEditor && GEditor->PlayWorld != nullptr)
	{
		// PIE/SIE 플레이 종료
		GEditor->RequestEndPlayMap();
	}
	else
	{
		auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
		if (IsValid(pc))
		{
			UKismetSystemLibrary::QuitGame(this, pc, EQuitPreference::Quit, false);
		}
	}
#endif
}
