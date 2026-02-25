// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_DateTime.h"
#include "RulesHorrorUtils.h"
#include "UI/RulesHorrorWidgetHelper.h"

void UUI_DateTime::NativePreConstruct()
{
	Super::NativePreConstruct();

	UpdateDateTime();
}

void UUI_DateTime::NativeConstruct()
{
	Super::NativeConstruct();

	auto world = GetWorld();
	if (IsValid(world))
	{
		FTimerHandle handle;
		world->GetTimerManager().SetTimer(handle, this, &UUI_DateTime::UpdateDateTime, 20.0f, true);
	}
}

void UUI_DateTime::OverwriteDateTime(const FDateTime& _overwrite_date_time, bool _overwrite_date, bool _overwrite_time)
{
	_OverwriteDate = _overwrite_date;
	_OverwriteTime = _overwrite_time;

	_OverwriteDateTime = _overwrite_date_time;

	UpdateDateTime();
}

void UUI_DateTime::UpdateDateTime()
{
	const FDateTime date_time_now = FDateTime::Now();

	// date
	if (_OverwriteDate)
	{
		SetDate(_OverwriteDateTime.GetYear(), _OverwriteDateTime.GetMonth(), _OverwriteDateTime.GetDay());
	}
	else
	{
		SetDate(date_time_now.GetYear(), date_time_now.GetMonth(), date_time_now.GetDay());
	}

	// time
	if (_OverwriteTime)
	{
		SetTime(_OverwriteDateTime.GetHour12(), _OverwriteDateTime.GetMinute(), _OverwriteDateTime.IsMorning());
	}
	else
	{
		SetTime(date_time_now.GetHour12(), date_time_now.GetMinute(), date_time_now.IsMorning());
	}
}
