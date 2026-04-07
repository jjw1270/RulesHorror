// Copyright (c) 2026 장윤제. All rights reserved.


#include "Lobby/Computer.h"
#include "RulesHorrorUtils.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UI/Lobby/UI_Monitor.h"
#include "UI/Lobby/UI_OnInteractingComputer.h"

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

	_OnMoveToPointFinishedEvent.BindDynamic(this, &AComputer::OnLobbyPawnMoveToPointFinished);
	
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

	// 모니터 스크린
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

void AComputer::SetInteractionState_Implementation(EInteractionState _state)
{
	_InteractionState = _state;
}

void AComputer::Interact_Implementation(AActor* _interactor)
{
	_InteractorLobbyPawn = Cast<ALobbyPawn>(_interactor);
	if (IsInvalid(_InteractorLobbyPawn))
		return;

	_InteractorLobbyPawn->SetTargetMovePoint(_InteractMovePoint, _OnMoveToPointFinishedEvent);

	_InteractorLobbyPawn->SetInteractingComputer(this);
	SetPower(true, true);
}

void AComputer::FinishInteract()
{
	if (IsValid(_InteractorLobbyPawn))
	{
		_InteractorLobbyPawn->SetTargetMovePoint(_FinishInteractMovePoint, _OnMoveToPointFinishedEvent);

		_InteractorLobbyPawn->SetInteractingComputer(nullptr);
		CloseInteractingWidget();
	}
}

void AComputer::OnLobbyPawnMoveToPointFinished(const FName& _point_name)
{
	if (IsInvalid(_InteractorLobbyPawn))
		return;

	// material 설정은 변경 해야 할수도(연출 요소)

	if (_point_name == _InteractMovePoint)
	{
		OpenInteractingWidget();
	}
	else if (_point_name == _FinishInteractMovePoint)
	{

	}
}

void AComputer::OpenInteractingWidget()
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsInvalid(pc))
		return;

	if (IsInvalid(_OnInteractingWidget))
	{
		if (IsValid(_OnInteractingWidgetClass))
		{
			_OnInteractingWidget = CreateWidget<UUI_OnInteractingComputer>(pc, _OnInteractingWidgetClass);
		}
		else
		{
			TRACE_ERROR(TEXT("_OnInteractingWidgetClass is invalid."));
		}
	}

	if (IsInvalid(_OnInteractingWidget))
		return;

	if (_OnInteractingWidget->IsInViewport() == false)
	{
		_OnInteractingWidget->SetComputer(this);
		_OnInteractingWidget->AddToViewport((int32)ERulesHorrorWidgetZOrder::Page);
	}
}

void AComputer::CloseInteractingWidget()
{
	if (IsInvalid(_OnInteractingWidget))
		return;

	_OnInteractingWidget->Close();
}
