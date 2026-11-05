// Copyright (c) 2026 장윤제. All rights reserved.


#include "Computer.h"
#include "RulesHorrorUtils.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UI/Office/UI_Monitor.h"
#include "UI/Office/UI_OnInteractingComputer.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"

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

	_OnMoveToPointFinishedEvent.BindDynamic(this, &AComputer::OnOfficePawnMoveToPointFinished);
	
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

	// 레벨을 열자마자 스토리가 전원을 켰으면(이어하기 직후) 화면 위젯은 이미 켜지는 애니메이션 중이다.
	// 다시 Show를 부르면 애니메이션이 첫 키로 멈춰 화면이 안 보이므로 머티리얼만 켠다.
	if (_IsPowerOn)
	{
		_DynamicScreenMaterialInstance->SetScalarParameterValue(FilterParamName, 1.0f);
		return;
	}

	SetPower(false, false);
}

void AComputer::SetPower(bool _on, bool _show_anim)
{
	_IsPowerOn = _on;

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
	_OfficePawn = Cast<AOfficePawn>(_interactor);
	if (IsInvalid(_OfficePawn))
		return;

	_OfficePawn->SetTargetMovePoint(_InteractMovePoint, false, _OnMoveToPointFinishedEvent);

	_OfficePawn->SetInteractingComputer(this);
	SetPower(true, true);
}

bool AComputer::CanBeDetected_Implementation() const
{
	return _IsInteractionEnabled && IsInvalid(_OfficePawn);
}

bool AComputer::CanInteract_Implementation() const
{
	return _IsInteractionEnabled;
}

void AComputer::FinishInteract()
{
	if (IsValid(_OfficePawn))
	{
		_OfficePawn->SetTargetMovePoint(_FinishInteractMovePoint, false, _OnMoveToPointFinishedEvent);

		_OfficePawn->SetInteractingComputer(nullptr);
		CloseInteractingWidget();

		_OfficePawn = nullptr;
	}
}

void AComputer::OnOfficePawnMoveToPointFinished(const FName& _point_name)
{
	if (IsInvalid(_OfficePawn))
		return;

	// material 설정은 변경 해야 할수도(연출 요소)

	if (_point_name == _InteractMovePoint)
	{
		OpenInteractingWidget();
		URulesHorrorStorySubsystem::Send(this, RulesHorrorStoryEvents::ComputerOpened);
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
