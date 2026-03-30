// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Computer.h"
#include "RulesHorrorUtils.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"

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

	SetPower(false);
}

void AComputer::SetPower(bool _on)
{
	if (IsValid(_DynamicScreenMaterialInstance))
	{
		_DynamicScreenMaterialInstance->SetScalarParameterValue(FilterParamName, _on ? 1.0f : 0.0f);
	}

	auto widget = (ScreenWidgetComponent->GetWidget());
	if (IsValid(widget))
	{
		widget->SetRenderOpacity(_on ? 1.0f : 0.0f);
	}
}
