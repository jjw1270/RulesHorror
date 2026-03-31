// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Computer.h"
#include "RulesHorrorUtils.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UI/Lobby/UI_Monitor.h"

AComputer::AComputer()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MonitorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Monitor"));
	MonitorMesh->SetupAttachment(Root);

	ScreenWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ScreenWidget"));
	ScreenWidgetComponent->SetupAttachment(MonitorMesh);
}

void AComputer::BeginPlay()
{
	Super::BeginPlay();
	
	GetWorldTimerManager().SetTimerForNextTick(this, &AComputer::UpdateScreenFromWidget);
}

void AComputer::UpdateScreenFromWidget()
{
	if (IsInvalid(_DynamicScreenMaterialInstance))
	{
		if (IsValid(_ScreenMaterialInstance))
		{
			_DynamicScreenMaterialInstance = UMaterialInstanceDynamic::Create(_ScreenMaterialInstance, this);

			MonitorMesh->SetMaterial(1, _DynamicScreenMaterialInstance);
		}
	}

	if (IsInvalid(_DynamicScreenMaterialInstance))
		return;

	auto widget_render_target = ScreenWidgetComponent->GetRenderTarget();
	if (IsValid(widget_render_target))
	{
		_DynamicScreenMaterialInstance->SetTextureParameterValue(TextureParamName, widget_render_target);
	}

	SetPower(false, false);
}

void AComputer::SetPower(bool _on, bool _show_anim)
{
	if (IsValid(_DynamicScreenMaterialInstance))
	{
		_DynamicScreenMaterialInstance->SetScalarParameterValue(FilterParamName, _on ? 1.0f : 0.0f);
	}

	auto monitor_widget = Cast<UUI_Monitor>(ScreenWidgetComponent->GetWidget());
	if (IsValid(monitor_widget))
	{
		if (_on)
		{
			monitor_widget->Show(EWidgetShowType::SelfHitTestInvisible, !_show_anim);
		}
		else
		{
			monitor_widget->Hide(EWidgetHideType::Collapsed, !_show_anim);
		}
	}
}
