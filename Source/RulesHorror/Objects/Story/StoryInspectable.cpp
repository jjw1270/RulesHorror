// Copyright (c) 2026 장윤제. All rights reserved.


#include "Objects/Story/StoryInspectable.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CommonUtils.h"

namespace
{
	// DefaultEngine.ini의 오브젝트 채널: "Interactable"(조사 대상), "Interactor"(폰의 감지 구, InteractorComponent)
	static const ECollisionChannel InteractableObjectChannel = ECC_GameTraceChannel1;
	static const ECollisionChannel InteractorObjectChannel = ECC_GameTraceChannel2;
}

AStoryInspectable::AStoryInspectable()
{
	PrimaryActorTick.bCanEverTick = false;

	// 감지용 박스: Interactable 오브젝트 타입으로 폰의 감지 구와 겹치고(사무실 폰은 Interactor 채널, C++ 기본값은 WorldDynamic),
	// 가시성 검사(Visibility 라인 트레이스)가 이 박스에 맞아야 보이는 것으로 친다.
	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	SetRootComponent(InteractBox);
	InteractBox->InitBoxExtent(FVector(12.0f, 12.0f, 4.0f));
	InteractBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractBox->SetCollisionObjectType(InteractableObjectChannel);
	InteractBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractBox->SetCollisionResponseToChannel(InteractorObjectChannel, ECR_Overlap);
	InteractBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	InteractBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractBox->SetGenerateOverlapEvents(true);
	InteractBox->SetLineThickness(0.5f);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(InteractBox);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
}

void AStoryInspectable::SetInspectEnabled(bool _is_enabled)
{
	_IsInspectEnabled = _is_enabled;
}

void AStoryInspectable::SetInteractionState_Implementation(EInteractionState _state)
{
	_InteractionState = _state;
}

bool AStoryInspectable::CanBeDetected_Implementation() const
{
	// 대사 중이나 입력 잠금 중에는 상호작용 표시도 띄우지 않는다.
	return CanInteract_Implementation();
}

bool AStoryInspectable::CanInteract_Implementation() const
{
	if (_IsInspectEnabled == false)
	{
		return false;
	}

	const URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this);
	return IsInvalid(story_subsystem) || (story_subsystem->IsDialoguePlaying() == false && story_subsystem->IsPlayerInputLocked() == false);
}

void AStoryInspectable::Interact_Implementation(AActor* _interactor)
{
	URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this);
	if (IsInvalid(story_subsystem) || _Lines.Num() == 0)
	{
		OnInspectDialogueFinished();
		return;
	}

	story_subsystem->PlayDialogue(_Lines, FOnStoryDialogueFinished::CreateUObject(this, &AStoryInspectable::OnInspectDialogueFinished));
}

FText AStoryInspectable::GetDisplayName_Implementation() const
{
	return _InteractionName.IsEmpty() ? FText::FromString(TEXT("조사하기")) : _InteractionName;
}

FVector AStoryInspectable::GetInteractionLocation_Implementation() const
{
	return InteractBox->GetComponentLocation();
}

TSet<UMeshComponent*> AStoryInspectable::GetEffectedMeshComponents_Implementation() const
{
	TSet<UMeshComponent*> mesh_components;
	if (IsValid(Mesh) && IsValid(Mesh->GetStaticMesh()))
	{
		mesh_components.Add(Mesh);
	}

	return mesh_components;
}

void AStoryInspectable::OnInspectDialogueFinished()
{
	_IsInspected = true;
	URulesHorrorStorySubsystem::Send(this, _EventName);
}
