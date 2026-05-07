# InteractionSystem User Guide

이 문서는 InteractionSystem 플러그인을 실제 프로젝트에서 사용하는 사람을 위한 작업 가이드다. 코드 구조 설명보다 **무엇을 만들고, 어디에 설정하고, 어떻게 입력/상태/UI를 연결하는지**에 초점을 둔다.

---

## 1. 기본 개념

InteractionSystem은 다음 단위로 상호작용을 구성한다.

| 이름 | 설명 |
| --- | --- |
| Interactor | 상호작용을 시도하는 Pawn/Character. `InteractorComponent`를 붙인다. |
| Interactable | 상호작용 대상 Actor. `InteractableInterface`를 구현한다. |
| Detectable Range | 주변 후보로 감지되는 최대 거리 |
| Targetable Range | 실제 상호작용 대상으로 선택될 수 있는 거리 |
| Detect Mode | 화면 중앙 기준 또는 마우스 커서 기준 감지 방식 |
| Indicator Panel | 감지된 Actor들의 화면 위치 아이콘과 Targeted UI를 표시하는 Widget |
| Overlay Material | Targeted Actor Mesh에 적용되는 외곽선/강조 Material |

가장 흔한 흐름은 다음과 같다.

```text
Project Settings 설정
-> 플레이어 Pawn에 InteractorComponent 추가
-> 상호작용 Actor가 InteractableInterface 구현
-> 입력에서 InteractorComponent::TryInteract 호출
-> Detected/Targeted 상태에 따라 UI와 Overlay 표시
```

---

## 2. 처음 설정하기

### 2.1 플러그인 활성화

`InteractionSystem.uplugin`은 다음 플러그인에 의존한다.

- `CommonLibrary`
- `CustomUI`

프로젝트에서 InteractionSystem을 사용하려면 위 플러그인들도 함께 활성화되어 있어야 한다.

### 2.2 Project Settings 설정

Project Settings에서 InteractionSystem 설정을 찾고 Overlay Material을 지정한다.

| 설정 | 필수 | 설명 |
| --- | --- | --- |
| OverlayMaterialClass | 권장/사실상 필수 | Targeted Actor Mesh에 적용할 Overlay Material |

현재 프로젝트 기본값:

```ini
[/Script/InteractionSystem.InteractionSystemDeveloperSettings]
_OverlayMaterialClass=/InteractionSystem/Materials/MI_OverlayOutline.MI_OverlayOutline
```

플러그인 기본 콘텐츠를 사용하려면 Content Browser에서 **Show Plugin Content**를 켜고 다음 에셋을 확인한다.

```text
/InteractionSystem/Materials/MI_OverlayOutline
```

주의:

- 현재 코드에서는 Overlay Material 설정이 비어 있으면 BeginPlay에서 에러 로그를 출력한다.
- Overlay를 전혀 사용하지 않는 프로젝트라면 코드 정책을 별도로 완화하는 것이 좋다.

---

## 3. Interactor 만들기

### 3.1 Pawn/Character에 Component 추가

플레이어 Pawn 또는 Character에 `InteractorComponent`를 추가한다.

C++ 예:

```cpp
// .h
UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
TObjectPtr<class UInteractorComponent> InteractorComponent = nullptr;
```

```cpp
// .cpp
#include "InteractorComponent.h"

InteractorComponent = CreateDefaultSubobject<UInteractorComponent>(TEXT("Interactor"));
InteractorComponent->SetupAttachment(GetRootComponent());
```

Blueprint로도 Component를 추가할 수 있다.

### 3.2 InteractorComponent 필수 설정

Details에서 다음을 설정한다.

| 설정 | 추천 기준 |
| --- | --- |
| OverlapObjectTypes | 상호작용 Actor가 사용하는 Object Type을 추가한다. 비어 있으면 동작하지 않는다. |
| DetectMode | FPS/3인칭 화면 중앙이면 `CameraCenter`, 마우스 클릭 기반이면 `Cursor` |
| DetectableRange | 감지 후보로 잡을 거리. 예: 600 |
| TargetableRange | 실제 상호작용 가능 거리. 예: 300 |
| MaxViewHalfAngleDegrees | CameraCenter 모드의 화면 중앙 허용 각도. 예: 30~60 |
| CursorDetectRadius | Cursor 모드의 화면 픽셀 감지 반경. 예: 80~160 |
| IndicatorPanelClass | Indicator UI를 쓸 경우 Panel Widget Blueprint 지정 |
| IndicatorPanelZOrder | Viewport 표시 순서 |

중요:

- `DetectableRange >= TargetableRange`가 되도록 설정한다.
- `OverlapObjectTypes`는 대상 Actor의 Collision Object Type과 맞아야 한다.
- InteractorComponent는 `USphereComponent`이므로 Collision이 Overlap되도록 대상 Actor Collision 설정도 맞춰야 한다.

### 3.3 Detect Mode 선택 기준

| 상황 | 추천 DetectMode |
| --- | --- |
| FPS/3인칭에서 화면 중앙 조준으로 상호작용 | `CameraCenter` |
| 마우스 커서로 오브젝트를 클릭/선택 | `Cursor` |
| 특정 상황에서 상호작용 잠금 | `NA` |

DetectMode를 런타임에 바꿀 수 있다.

```cpp
InteractorComponent->SetDetectMode(EInteractionDetectMode::NA);
InteractorComponent->SetDetectMode(EInteractionDetectMode::CameraCenter);
```

`NA`로 바꾸면 현재 상태가 초기화되고 Tick이 꺼진다.

---

## 4. Interactable Actor 만들기

### 4.1 Blueprint에서 만들기

1. 상호작용 대상 Actor Blueprint를 연다.
2. Class Settings에서 `InteractableInterface`를 추가한다.
3. 필요한 Interface 함수를 구현한다.

최소 구현:

| 함수 | 목적 |
| --- | --- |
| `Interact` | 플레이어가 상호작용했을 때 실행할 로직 |
| `GetDisplayName` | Target UI에 보여줄 이름 |

선택 구현:

| 함수 | 필요할 때 |
| --- | --- |
| `CanInteract` | 잠금/쿨다운/조건부 상호작용 |
| `CanBeDetected` | 상태에 따라 감지 자체를 숨길 때 |
| `SetInteractionState` | Detected/Targeted 상태에 따라 자체 연출을 바꿀 때 |
| `GetInteractionLocation` | Actor 중심이 아닌 소켓/위젯/핸들 위치를 기준으로 삼을 때 |
| `GetEffectedMeshComponents` | Overlay를 특정 Mesh에만 적용하고 싶을 때 |
| `CanBeCollapsedBySelf` | 자신의 Mesh가 InteractionLocation을 가릴 수 있는 구성을 엄격하게 처리할 때 |

### 4.2 C++에서 만들기

```cpp
#include "InteractableInterface.h"

UCLASS()
class AMyDoor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	virtual FText GetDisplayName_Implementation() const override
	{
		return FText::FromString(TEXT("Door"));
	}

	virtual bool CanInteract_Implementation() const override
	{
		return bIsLocked == false;
	}

	virtual void Interact_Implementation(AActor* _interactor) override
	{
		OpenDoor();
	}
};
```

Interaction 위치를 별도 SceneComponent 기준으로 쓰고 싶다면:

```cpp
virtual FVector GetInteractionLocation_Implementation() const override
{
	return IsValid(InteractionPoint)
		? InteractionPoint->GetComponentLocation()
		: GetActorLocation();
}
```

Overlay 대상 Mesh를 제한하고 싶다면:

```cpp
virtual TSet<UMeshComponent*> GetEffectedMeshComponents_Implementation() const override
{
	TSet<UMeshComponent*> Result;

	if (IsValid(DoorMesh))
	{
		Result.Add(DoorMesh);
	}

	return Result;
}
```

---

## 5. 입력 연결하기

### 5.1 C++에서 TryInteract 호출

`UInteractorComponent::TryInteract()`는 현재 코드 기준 BlueprintCallable이 아니므로 C++ 입력 함수에서 호출하는 방식이 기본이다.

```cpp
void AMyCharacter::OnInteract()
{
	if (IsValid(InteractorComponent))
	{
		InteractorComponent->TryInteract();
	}
}
```

Enhanced Input 사용 예:

```cpp
EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Started, this, &AMyCharacter::OnInteract);
```

### 5.2 DetectMode별 입력 분리 예시

화면 중앙 상호작용과 커서 클릭 상호작용을 분리하고 싶다면:

```cpp
void AMyCharacter::OnKeyboardInteract()
{
	if (IsValid(InteractorComponent) &&
		InteractorComponent->GetDetectMode() == EInteractionDetectMode::CameraCenter)
	{
		InteractorComponent->TryInteract();
	}
}

void AMyCharacter::OnMouseInteract()
{
	if (IsValid(InteractorComponent) &&
		InteractorComponent->GetDetectMode() == EInteractionDetectMode::Cursor)
	{
		InteractorComponent->TryInteract();
	}
}
```

### 5.3 Blueprint에서 호출하고 싶을 때

현재 `TryInteract()`는 BlueprintCallable이 아니다. Blueprint에서 직접 호출하려면 프로젝트 Pawn/Character에 래퍼 함수를 만든다.

```cpp
UFUNCTION(BlueprintCallable)
void TryInteractFromBlueprint()
{
	if (IsValid(InteractorComponent))
	{
		InteractorComponent->TryInteract();
	}
}
```

---

## 6. Indicator UI 만들기

플러그인에는 샘플 Widget이 포함되어 있다.

| 에셋 | 용도 |
| --- | --- |
| `UI_InteractionIndicator_BP` | 개별 Detected 아이콘 |
| `UI_InteractionIndicatorPanel_BP` | 여러 아이콘과 Targeted 표시를 관리하는 Panel |

### 6.1 Indicator Widget

부모 클래스:

```text
UI_InteractionIndicator
```

주요 구현 포인트:

- `SetDetectedIconSize` 이벤트에서 아이콘 크기를 UI에 반영한다.
- 필요하면 `SetActorState`를 Override해서 `Detected`, `Targeted`, `None` 상태별 애니메이션을 바꾼다.

기본 동작:

| 상태 | 기본 표시 |
| --- | --- |
| `None` | 숨김 |
| `Detected` | 아이콘 표시 |
| `Targeted` | 아이콘 숨김 |

Targeted 상태에서 개별 아이콘이 숨겨지는 이유는 Panel의 Targeted UI가 별도로 표시되기 때문이다.

### 6.2 Indicator Panel Widget

부모 클래스:

```text
UI_InteractionIndicatorPanel
```

필수 위젯:

| 이름 | 타입 | 설명 |
| --- | --- | --- |
| `CP_Indicators` | `CanvasPanel` | 개별 Indicator들이 붙는 Canvas |

Details 설정:

| 설정 | 설명 |
| --- | --- |
| IndicatorClass | `UI_InteractionIndicator`를 상속한 Widget Class |
| IndicatorSize | 개별 Indicator 크기 |
| IndicatorPoolSize | 초기에 만들어둘 Indicator 수 |
| PerspectiveMinScale | 먼 거리에서 줄어드는 최소 스케일 |

Blueprint에서 구현할 이벤트:

| 이벤트 | 설명 |
| --- | --- |
| `ShowTargetedPanel(bool Show, FText DisplayName)` | Targeted Actor가 바뀔 때 이름 UI 표시/숨김 |

예시 흐름:

```text
ShowTargetedPanel(true, "Door")
-> 중앙 안내 텍스트 또는 상호작용 프롬프트 표시

ShowTargetedPanel(false, Empty)
-> 프롬프트 숨김
```

### 6.3 InteractorComponent에 Panel 지정

플레이어 Pawn의 `InteractorComponent` Details에서:

```text
IndicatorPanelClass = UI_InteractionIndicatorPanel_BP
```

Panel은 로컬 컨트롤러에서만 생성되어 Viewport에 추가된다.

---

## 7. Overlay Material 설정

### 7.1 기본 Overlay 사용

Project Settings의 `OverlayMaterialClass`에 다음 기본 Material Instance를 지정한다.

```text
/InteractionSystem/Materials/MI_OverlayOutline
```

Targeted Actor가 생기면 해당 Actor의 `GetEffectedMeshComponents()` 결과에 Overlay Material이 적용된다.

### 7.2 Overlay 대상 제한

Actor에 여러 Mesh가 있고 일부 Mesh만 강조하고 싶으면 `GetEffectedMeshComponents`를 구현한다.

예:

```cpp
virtual TSet<UMeshComponent*> GetEffectedMeshComponents_Implementation() const override
{
	TSet<UMeshComponent*> Meshes;
	Meshes.Add(InteractableMesh);
	return Meshes;
}
```

### 7.3 Nanite 주의

Static Mesh Overlay Material은 Nanite와 함께 사용할 수 없으므로, InteractionSystem은 Overlay 적용 중 해당 StaticMeshComponent의 Nanite를 강제로 비활성화한다.

현재 정책:

- Overlay 적용 전 원래 `ForceDisableNanite` 값을 저장한다.
- Overlay 해제 시 원래 값으로 복원한다.
- 이 처리는 로컬 컨트롤러에서만 수행된다.

---

## 8. Collision 설정 체크리스트

상호작용이 감지되지 않으면 먼저 Collision을 확인한다.

### InteractorComponent

- `OverlapObjectTypes`에 대상 Actor의 Object Type이 들어 있는가?
- Sphere Radius가 `DetectableRange`로 충분히 큰가?
- DetectMode가 `NA`가 아닌가?

### Interactable Actor

- Actor Class가 `InteractableInterface`를 구현했는가?
- 대상 Primitive가 InteractorComponent와 Overlap 가능한 Collision 설정인가?
- Cursor 모드라면 `GetHitResultUnderCursorForObjects`에 잡히는 Collision인가?
- Visibility trace가 막히지 않는가?

### Cursor 모드

- PlayerController가 마우스 위치를 제공할 수 있는 입력 모드인가?
- 마우스 커서가 표시/활성화되어 있는가?
- 대상이 `_CursorDetectRadius` 안에 화면 투영되는가?

---

## 9. Debug 사용하기

### 9.1 Component 설정

InteractorComponent의 `_ShowDebug`를 true로 켜면 Editor Visualizer와 런타임 Debug Drawing을 확인할 수 있다.

표시되는 정보:

- Detectable Range
- Targetable Range
- View 방향
- 감지 후보 Actor 위치
- CameraCenter 모드의 허용 시야 원
- Cursor 모드의 커서 반경 가이드

### 9.2 런타임 토글

C++에서:

```cpp
InteractorComponent->ToggleDebug();
```

주의:

- Debug Drawing은 Shipping 빌드에서 동작하지 않는다.
- `ToggleDebug()`는 Shipping에서 no-op이다.

---

## 10. 자주 생기는 문제

### 감지가 전혀 안 된다

확인 순서:

1. `OverlapObjectTypes`가 비어 있지 않은가?
2. 대상 Actor가 `InteractableInterface`를 구현했는가?
3. Collision Object Type과 Overlap 설정이 맞는가?
4. `DetectableRange` 안에 있는가?
5. `CanBeDetected()`가 true를 반환하는가?
6. Visibility trace가 막히지 않는가?

### 감지는 되는데 Targeted가 안 된다

확인 순서:

1. `TargetableRange` 안에 있는가?
2. `CameraCenter` 모드라면 화면 중앙 허용 각도 안에 있는가?
3. `Cursor` 모드라면 커서 Hit Result가 대상 Actor를 맞히는가?
4. `GetInteractionLocation()`이 실제로 원하는 위치인가?

### UI가 안 보인다

확인 순서:

1. `IndicatorPanelClass`가 지정되어 있는가?
2. Panel Widget에 `CP_Indicators` CanvasPanel이 정확한 이름으로 BindWidget 되어 있는가?
3. Panel의 `IndicatorClass`가 지정되어 있는가?
4. 로컬 플레이어 컨트롤러에서 실행 중인가?
5. CustomUI의 `WidgetBase` Show/Hide 규칙과 충돌하지 않는가?

### Overlay가 안 보인다

확인 순서:

1. Project Settings의 `OverlayMaterialClass`가 지정되어 있는가?
2. 대상 Actor의 `GetEffectedMeshComponents()`가 Mesh를 반환하는가?
3. 현재 Actor가 `Targeted` 상태인가?
4. 로컬 컨트롤러에서 실행 중인가?
5. Material이 Overlay Material 용도로 정상 구성되어 있는가?

### Interact가 호출되지 않는다

확인 순서:

1. 입력에서 `TryInteract()`를 실제로 호출하는가?
2. 현재 `GetTargetedActor()`가 유효한가?
3. 대상 Actor의 `CanInteract()`가 true인가?
4. `Interact` 구현이 Blueprint/C++에 제대로 들어가 있는가?

---

## 11. 권장 제작 순서

처음 붙일 때는 다음 순서로 진행한다.

1. 기본 Overlay Material을 Project Settings에 지정한다.
2. 플레이어 Pawn에 `InteractorComponent`를 추가한다.
3. `OverlapObjectTypes`, `DetectableRange`, `TargetableRange`, `DetectMode`를 설정한다.
4. `InteractableInterface`를 구현한 간단한 테스트 Actor를 만든다.
5. 입력에서 `TryInteract()`를 호출한다.
6. `_ShowDebug`로 감지 범위와 Target 선택을 확인한다.
7. `UI_InteractionIndicatorPanel_BP`를 연결한다.
8. 프로젝트용 Indicator/Targeted UI 디자인으로 교체한다.
9. 각 상호작용 Actor에 `GetInteractionLocation`, `CanInteract`, `GetEffectedMeshComponents`를 필요에 맞게 구현한다.

