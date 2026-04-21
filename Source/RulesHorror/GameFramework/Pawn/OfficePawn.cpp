// Copyright (c) 2026 장윤제. All rights reserved.


#include "OfficePawn.h"
#include "RulesHorrorUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InteractorComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "Objects/Interactable/Computer.h"
#include "UI/Office/UI_Monitor.h"
#include "WidgetHelper.h"

AOfficePawn::AOfficePawn()
{
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	WidgetInteractionComponent = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("WidgetInteraction"));
	WidgetInteractionComponent->SetupAttachment(GetRootComponent());

	WidgetInteractionComponent->InteractionSource = EWidgetInteractionSource::Custom;
	ApplyMonitorTraceChannel();
}

void AOfficePawn::BeginPlay()
{
	Super::BeginPlay();

	ApplyMonitorTraceChannel();
}

void AOfficePawn::SetupPlayerInputComponent(UInputComponent* _input_component)
{
	Super::SetupPlayerInputComponent(_input_component);

	auto enhanced_input_comp = Cast<UEnhancedInputComponent>(_input_component);
	if (IsInvalid(enhanced_input_comp))
	{
		TRACE_ERROR(TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
		return;
	}

	if (IsAnyInvalid(_IA_LeftMouseClick, _IA_MouseWheel))
	{
		TRACE_ERROR(TEXT("Some Input Action is not set!!"));
		return;
	}

	enhanced_input_comp->BindAction(_IA_LeftMouseClick, ETriggerEvent::Started, this, &AOfficePawn::Input_LeftMouseButtonStarted);
	enhanced_input_comp->BindAction(_IA_LeftMouseClick, ETriggerEvent::Completed, this, &AOfficePawn::Input_LeftMouseButtonCompleted);
	enhanced_input_comp->BindAction(_IA_MouseWheel, ETriggerEvent::Triggered, this, &AOfficePawn::Input_MouseWheel);
}

void AOfficePawn::Tick(float _delta_time)
{
	Super::Tick(_delta_time);

	DriveWidgetInteraction(_delta_time);
}

void AOfficePawn::Input_LeftMouseButtonStarted()
{
	if (IsValid(WidgetInteractionComponent->GetHoveredWidgetComponent()))
	{
		WidgetInteractionComponent->PressPointerKey(EKeys::LeftMouseButton);
	}
}

void AOfficePawn::Input_LeftMouseButtonCompleted()
{
	if (IsValid(WidgetInteractionComponent->GetHoveredWidgetComponent()))
	{
		WidgetInteractionComponent->ReleasePointerKey(EKeys::LeftMouseButton);
	}
}

void AOfficePawn::Input_MouseWheel(const FInputActionValue& _value)
{
	const float axis = _value.Get<float>();
	if (FMath::IsNearlyZero(axis))
		return;

	if (IsValid(WidgetInteractionComponent->GetHoveredWidgetComponent()))
	{
		WidgetInteractionComponent->ScrollWheel(axis);
	}
}

void AOfficePawn::DriveWidgetInteraction(float _delta_time)
{
	if (IsInvalid(_InteractingComputer))
		return;

	auto pc = Cast<APlayerController>(GetController());
	if (IsInvalid(pc))
		return;

	FHitResult hit;
	bool is_hit = pc->GetHitResultUnderCursorByChannel(_MonitorScreenWidgetTraceType, true, hit);

	pc->SetShowMouseCursor(!is_hit);

	BuildCorrectedMonitorWidgetHit(hit);
	WidgetInteractionComponent->SetCustomHitResult(hit);

	if (is_hit)
	{
		auto widget_component = Cast<UWidgetComponent>(hit.GetComponent());
		if (IsValid(widget_component))
		{
			// 이전에 이미 다른 위젯이 존재할경우
			auto new_interacting_monitor_widget = Cast<UUI_Monitor>(widget_component->GetWidget());
			if (IsValid(_InteractingMonitorWidget) && _InteractingMonitorWidget != new_interacting_monitor_widget)
			{
				_InteractingMonitorWidget->SetRealMousePointerHovered(false);
			}

			_InteractingMonitorWidget = new_interacting_monitor_widget;
			if (IsValid(_InteractingMonitorWidget))
			{
				_InteractingMonitorWidget->SetRealMousePointerHovered(true);
				_InteractingMonitorWidget->SetMonitorCursorPosition(WidgetInteractionComponent->Get2DHitLocation());
			}
		}
	}
	else
	{
		if (IsValid(_InteractingMonitorWidget))
		{
			_InteractingMonitorWidget->SetRealMousePointerHovered(false);
			_InteractingMonitorWidget = nullptr;
		}
	}
}

void AOfficePawn::BuildCorrectedMonitorWidgetHit(FHitResult& _out_hit) const
{
	if (_out_hit.HasValidHitObjectHandle() == false)
		return;

	auto widget_component = Cast<UWidgetComponent>(_out_hit.GetComponent());
	if (IsInvalid(widget_component))
		return;

	const FVector2D draw_size = FVector2D(widget_component->GetDrawSize());
	if (draw_size.X <= 1.0f || draw_size.Y <= 1.0f)
		return;

	const FVector2D pivot = widget_component->GetPivot();

	const FTransform component_transform = widget_component->GetComponentTransform();
	const FVector local_hit_location = component_transform.InverseTransformPosition(_out_hit.ImpactPoint);

	// WidgetComponent 기준:
	// X = 평면 법선 방향
	// Y = 좌우
	// Z = 상하
	//
	// local -> uv
	// U: 좌(0) ~ 우(1)
	// V: 상(0) ~ 하(1)
	float uv_x = (local_hit_location.Y / draw_size.X) + pivot.X;
	float uv_y = pivot.Y - (local_hit_location.Z / draw_size.Y);

	uv_x = FMath::Clamp(uv_x, 0.0f, 1.0f);
	uv_y = FMath::Clamp(uv_y, 0.0f, 1.0f);

	// 중심 기준 -1 ~ +1
	float normalized_x = (uv_x - 0.5f) * 2.0f;
	float normalized_y = (uv_y - 0.5f) * 2.0f;

	normalized_x = FMath::Clamp(normalized_x, -1.0f, 1.0f);
	normalized_y = FMath::Clamp(normalized_y, -1.0f, 1.0f);

	// 반경 기반 + 축 분리 보정
	const float r2 = FMath::Min((normalized_x * normalized_x) + (normalized_y * normalized_y), 2.0f);

	float correction_scale_x = 1.0f - (_MonitorHitCorrectionStrength.X * r2);
	float correction_scale_y = 1.0f - (_MonitorHitCorrectionStrength.Y * r2);

	correction_scale_x = FMath::Clamp(correction_scale_x, 0.7f, 1.0f);
	correction_scale_y = FMath::Clamp(correction_scale_y, 0.7f, 1.0f);

	const float corrected_normalized_x = normalized_x * correction_scale_x;
	const float corrected_normalized_y = normalized_y * correction_scale_y;

	// 다시 uv
	const float corrected_uv_x = FMath::Clamp((corrected_normalized_x * 0.5f) + 0.5f, 0.0f, 1.0f);
	const float corrected_uv_y = FMath::Clamp((corrected_normalized_y * 0.5f) + 0.5f, 0.0f, 1.0f);

	// uv -> local
	FVector corrected_local_hit_location = local_hit_location;
	corrected_local_hit_location.Y = (corrected_uv_x - pivot.X) * draw_size.X;
	corrected_local_hit_location.Z = (pivot.Y - corrected_uv_y) * draw_size.Y;

	const FVector corrected_world_hit_location = component_transform.TransformPosition(corrected_local_hit_location);

	_out_hit.Location = corrected_world_hit_location;
	_out_hit.ImpactPoint = corrected_world_hit_location;
	_out_hit.Distance = FVector::Distance(_out_hit.TraceStart, corrected_world_hit_location);
}

void AOfficePawn::ApplyMonitorTraceChannel()
{
	if (IsInvalid(WidgetInteractionComponent))
		return;

	WidgetInteractionComponent->TraceChannel = UEngineTypes::ConvertToCollisionChannel(_MonitorScreenWidgetTraceType);
}

void AOfficePawn::SetInteractingComputer(AComputer* _computer)
{
	_InteractingComputer = _computer;
}

bool AOfficePawn::IsOnInteracintingComputer() const
{
	return IsValid(_InteractingComputer);
}
