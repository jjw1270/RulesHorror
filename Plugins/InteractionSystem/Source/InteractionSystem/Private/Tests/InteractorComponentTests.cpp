#include "InteractorComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractorComponentDetectableRangeClampTest,
	"InteractionSystem.InteractorComponent.DetectableRangeClampsToTargetableRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractorComponentDetectableRangeClampTest::RunTest(const FString& Parameters)
{
	UInteractorComponent* interactor_component = NewObject<UInteractorComponent>();
	TestNotNull(TEXT("InteractorComponent should be created"), interactor_component);
	if (interactor_component == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("Default sphere radius should match the default detectable range"), interactor_component->GetUnscaledSphereRadius(), 600.0f);

	AddExpectedError(TEXT("상호작용 가능 거리가 감지거리보다 클 수 없습니다!"), EAutomationExpectedErrorFlags::Contains, 2);
	AddExpectedError(TEXT("감지거리가 상호작용 가능 거리보다 작을 수 없습니다!"), EAutomationExpectedErrorFlags::Contains, 2);

	interactor_component->SetTargetableRange(700.0f);
	interactor_component->SetDetectableRange(500.0f);

	TestEqual(TEXT("Detectable range should clamp up to the already-clamped targetable range"), interactor_component->GetUnscaledSphereRadius(), 600.0f);

	interactor_component->SetTargetableRange(400.0f);
	interactor_component->SetDetectableRange(450.0f);
	TestEqual(TEXT("Valid detectable range should update the sphere radius"), interactor_component->GetUnscaledSphereRadius(), 450.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractorComponentDetectModeSetterTest,
	"InteractionSystem.InteractorComponent.DetectModeSetterUpdatesState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractorComponentDetectModeSetterTest::RunTest(const FString& Parameters)
{
	UInteractorComponent* interactor_component = NewObject<UInteractorComponent>();
	TestNotNull(TEXT("InteractorComponent should be created"), interactor_component);
	if (interactor_component == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("Default detect mode should be CameraCenter"), interactor_component->GetDetectMode(), EInteractionDetectMode::CameraCenter);

	interactor_component->SetDetectMode(EInteractionDetectMode::Cursor);
	TestEqual(TEXT("Detect mode should switch to Cursor"), interactor_component->GetDetectMode(), EInteractionDetectMode::Cursor);

	interactor_component->SetDetectMode(EInteractionDetectMode::NA);
	TestEqual(TEXT("Detect mode should switch to NA"), interactor_component->GetDetectMode(), EInteractionDetectMode::NA);
	TestFalse(TEXT("NA detect mode should disable ticking when there are no overlapped actors"), interactor_component->IsComponentTickEnabled());

	return true;
}

#endif
