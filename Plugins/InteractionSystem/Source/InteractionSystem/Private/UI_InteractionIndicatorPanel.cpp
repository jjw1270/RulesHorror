// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_InteractionIndicatorPanel.h"
#include "CommonUtils.h"
#include "UI_InteractionIndicator.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InteractableInterface.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"

void UUI_InteractionIndicatorPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// init pool
	if (IsValid(_IndicatorClass))
	{
		for (int32 i = 0; i < _IndicatorPoolSize; ++i)
		{
			auto widget = CreateWidget<UUI_InteractionIndicator>(this, _IndicatorClass);
			if (IsValid(widget))
			{
				widget->ClearWidget();

				_IndicatorPool.Add(widget);
			}
		}
	}

	ShowTargetedPanel(false, FText::GetEmpty());
}

void UUI_InteractionIndicatorPanel::NativeTick(const FGeometry& _geo, float _delta)
{
	Super::NativeTick(_geo, _delta);

	auto pc = GetOwningPlayer();
	if (IsInvalid(pc))
		return;

	FVector view_location = FVector::ZeroVector;
	FRotator view_rotation = FRotator::ZeroRotator;
	pc->GetPlayerViewPoint(view_location, view_rotation);

	for (auto it = _ActivatedIndicatorMap.CreateIterator(); it; ++it)
	{
		AActor* actor = it.Key();
		UUI_InteractionIndicator* indicator = it.Value();

		if (IsAnyInvalid(actor, indicator))
		{
			if (IsValid(indicator))
			{
				indicator->ClearWidget();
				indicator->Close(true);
			}

			it.RemoveCurrent();
			continue;
		}

		const EInteractionState interaction_state = indicator->GetActorState();

		const FVector world_location = IInteractableInterface::Execute_GetInteractionLocation(actor);

		FVector2D screen_position = FVector2D::ZeroVector;
		const bool is_projected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(pc, world_location, screen_position, false);

		if (is_projected == false)
		{
			indicator->SetRenderOpacity(0.0f);
			continue;
		}

		indicator->SetRenderOpacity(1.0f);

		// position
		auto cp_slot = Cast<UCanvasPanelSlot>(indicator->Slot);
		if (IsValid(cp_slot))
		{
			cp_slot->SetPosition(screen_position);
		}

		// scale
		const float distance = FVector::Dist(view_location, world_location);
		const float scale = FMath::IsNearlyEqual(_PerspectiveMinDistance, _PerspectiveMaxDistance)
			? 1.0f
			: FMath::GetMappedRangeValueClamped(FVector2D(_PerspectiveMinDistance, _PerspectiveMaxDistance), FVector2D(1.0f, _PerspectiveMinScale), distance);

		indicator->SetRenderScale(FVector2D(scale));
	}
}

void UUI_InteractionIndicatorPanel::NativeDestruct()
{
	for (auto& pair : _ActivatedIndicatorMap)
	{
		if (IsValid(pair.Value))
		{
			pair.Value->ClearWidget();
			pair.Value->Close(true);
		}
	}
	_ActivatedIndicatorMap.Empty();

	for (auto& indicator : _IndicatorPool)
	{
		if (IsValid(indicator))
		{
			indicator->ClearWidget();
			indicator->Close(true);
		}
	}
	_IndicatorPool.Empty();

	Super::NativeDestruct();
}

void UUI_InteractionIndicatorPanel::SetPerspectiveDistance(float _min_dist, float _max_dist)
{
	_PerspectiveMinDistance = _min_dist;
	_PerspectiveMaxDistance = _max_dist;
}

void UUI_InteractionIndicatorPanel::AddInteractionActor(AActor* _interaction_actor, EInteractionState _state)
{
	if(IsAnyInvalid(CP_Indicators, _interaction_actor))
		return;

	if (_ActivatedIndicatorMap.Contains(_interaction_actor))
	{
		TRACE_WARNING(TEXT("이미 Indicator가 존재합니다 : %s"), *_interaction_actor->GetName());
		return;
	}

	auto indicator = PickIndicatorFromPool();
	if (IsInvalid(indicator))
		return;

	indicator->SetDetectedIconSize(_IndicatorSize);
	indicator->SetInteractionActor(_interaction_actor, _state);
	indicator->SetRenderOpacity(0.0f);

	auto cp_slot = CP_Indicators->AddChildToCanvas(indicator);
	if (IsValid(cp_slot))
	{
		cp_slot->SetSize(_IndicatorSize);
		cp_slot->SetAlignment(FVector2D(0.5f, 0.5f));

		_ActivatedIndicatorMap.Add(_interaction_actor, indicator);
	}
}

void UUI_InteractionIndicatorPanel::RemoveInteractionActor(AActor* _interaction_actor)
{
	if (IsInvalid(_interaction_actor))
		return;

	auto indicator_ptr = _ActivatedIndicatorMap.Find(_interaction_actor);
	if (IsInvalid(indicator_ptr))
		return;

	auto indicator = * indicator_ptr;
	if (IsValid(indicator))
	{
		indicator->ClearWidget();
		indicator->Close();
	}

	_ActivatedIndicatorMap.Remove(_interaction_actor);
}

void UUI_InteractionIndicatorPanel::SetInteractionActorState(AActor* _interaction_actor, EInteractionState _state)
{
	if (IsInvalid(_interaction_actor))
		return;

	auto indicator_ptr = _ActivatedIndicatorMap.Find(_interaction_actor);
	if (IsInvalid(indicator_ptr))
		return;

	auto indicator = *indicator_ptr;
	if (IsInvalid(indicator))
		return;

	indicator->SetActorState(_state);
}

void UUI_InteractionIndicatorPanel::SetTargetedActor(AActor* _targeted_actor)
{
	if (_targeted_actor == _TargetedActor)
		return;
	_TargetedActor = _targeted_actor;

	if (IsValid(_TargetedActor))
	{
		ShowTargetedPanel(true, IInteractableInterface::Execute_GetDisplayName(_TargetedActor));
	}
	else
	{
		ShowTargetedPanel(false, FText::GetEmpty());
	}
}

UUI_InteractionIndicator* UUI_InteractionIndicatorPanel::PickIndicatorFromPool()
{
	for (auto indicator : _IndicatorPool)
	{
		if (IsInvalid(indicator))
			continue;

		if (IsValid(indicator->GetParent()))
			continue;

		return indicator;
	}

	// out of pool : 풀 확장
	if (IsValid(_IndicatorClass))
	{
		auto indicator = CreateWidget<UUI_InteractionIndicator>(this, _IndicatorClass);
		if (IsValid(indicator))
		{
			indicator->ClearWidget();
			_IndicatorPool.Add(indicator);

			return indicator;
		}
	}

	return nullptr;
}
