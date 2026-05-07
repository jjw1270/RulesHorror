# InteractionSystem

`InteractionSystem`은 Unreal Engine 5 프로젝트에서 플레이어/폰이 주변의 상호작용 가능 Actor를 감지하고, 바라보는 방향 또는 마우스 커서 기준으로 Target을 선택한 뒤, UI Indicator와 Overlay Material로 상태를 표시하고 `Interact` 호출까지 연결하는 런타임 상호작용 플러그인이다.

이 문서는 `Plugins/InteractionSystem`의 **현재 코드 기준**으로 유지되는 기술 README다. 실제 설정/제작 절차 중심 문서는 [UserGuide.md](./UserGuide.md)를 참고한다.

---

## 핵심 요약

- `UInteractorComponent`는 `USphereComponent` 기반 감지 컴포넌트다.
- 상호작용 대상 Actor는 `IInteractableInterface`를 구현해야 한다.
- 감지 방식은 `CameraCenter`와 `Cursor` 두 가지다.
- 감지 범위(`DetectableRange`)와 선택 가능 범위(`TargetableRange`)를 분리한다.
- Actor 상태는 `None`, `Detected`, `Targeted`로 관리된다.
- Targeted Actor에 대해서만 Overlay Material을 적용한다.
- Overlay/Indicator 같은 시각 요소는 로컬 컨트롤러에서만 표시하는 정책이다.
- `TryInteract()`는 현재 Targeted Actor의 `CanInteract`를 확인한 뒤 `Interact(GetOwner())`를 실행한다.
- Editor 모듈은 `UInteractorComponent`의 감지 범위와 방향을 Component Visualizer로 표시한다.

---

## 모듈 구성

플러그인은 `CommonLibrary`, `CustomUI` 플러그인에 의존한다.

| 모듈 | 역할 |
| --- | --- |
| `InteractionSystem` | 런타임 Interactor Component, Interactable Interface, Indicator UI, Overlay Material 처리 |
| `InteractionSystemEditor` | `UInteractorComponent` Component Visualizer 등록 및 에디터 디버그 시각화 |

`InteractionSystem.Build.cs`의 주요 의존 모듈:

- `Core`, `CoreUObject`, `Engine`
- `InputCore`
- `CommonLibrary`
- `CustomUI`
- `UMG`
- `DeveloperSettings`
- `Slate`, `SlateCore`(Private)

---

## 주요 런타임 타입

### `EInteractionDetectMode`

파일: `Source/InteractionSystem/Public/InteractionSystemDefines.h`

| 값 | 의미 |
| --- | --- |
| `NA` | 상호작용 감지 비활성 |
| `CameraCenter` | Player View 방향/화면 중앙 기준으로 Target 선택 |
| `Cursor` | 마우스 커서 위치와 커서 Hit Result 기준으로 Target 선택 |

### `EInteractionState`

파일: `Source/InteractionSystem/Public/InteractionSystemDefines.h`

| 값 | 의미 |
| --- | --- |
| `None` | 감지/선택되지 않음 |
| `Detected` | 감지 범위 안에 있고 가시성이 확인됨 |
| `Targeted` | 현재 상호작용 대상으로 선택됨 |

### `IInteractableInterface`

파일: `Source/InteractionSystem/Public/InteractableInterface.h`

상호작용 가능한 Actor가 구현해야 하는 인터페이스다.

| 함수 | 기본 동작 | 의미 |
| --- | --- | --- |
| `SetInteractionState(EInteractionState)` | 아무 작업 없음 | Interactor가 상태 변경 시 대상에게 통지 |
| `CanBeDetected()` | `true` | 감지 후보가 될 수 있는지 |
| `CanBeCollapsedBySelf()` | `false` | Visibility trace가 자신에게 막혔을 때도 가림으로 볼지 여부 |
| `CanInteract()` | `true` | 현재 상호작용 실행이 가능한지 |
| `Interact(AActor* _interactor)` | 아무 작업 없음 | 실제 상호작용 실행 |
| `GetDisplayName()` | `"Interact"` | Targeted UI에 표시할 이름 |
| `GetInteractionLocation()` | Actor 위치 | 거리/시야/화면 투영 기준점 |
| `GetEffectedMeshComponents()` | Actor의 모든 `UMeshComponent` | Overlay Material을 적용할 Mesh 목록 |

설계 의도:

- 간단한 Actor는 `Interact`와 `GetDisplayName`만 구현해도 동작한다.
- 정확한 감지점을 원하면 `GetInteractionLocation`을 소켓/컴포넌트 위치로 오버라이드한다.
- Overlay 대상 Mesh를 제한하려면 `GetEffectedMeshComponents`를 오버라이드한다.

### `UInteractorComponent`

파일: `Source/InteractionSystem/Public/InteractorComponent.h`

플레이어 Pawn/Character에 붙이는 감지 컴포넌트다. `USphereComponent`를 상속하므로 Overlap을 통해 주변 후보를 수집한다.

주요 설정:

| 설정 | 의미 |
| --- | --- |
| `_OverlapObjectTypes` | 감지할 Object Type 목록. 비어 있으면 BeginPlay에서 에러 로그를 낸다. |
| `_DetectMode` | `CameraCenter`, `Cursor`, `NA` 중 현재 감지 방식 |
| `_DetectableRange` | 후보 감지 최대 거리. Sphere Radius에도 반영된다. |
| `_TargetableRange` | 실제 Target 선택 가능 거리 |
| `_MaxViewHalfAngleDegrees` | `CameraCenter` 모드에서 View Forward 기준 허용 반각 |
| `_CursorDetectRadius` | `Cursor` 모드에서 화면상의 감지 반경(px) |
| `_ShowDebug` | Development/Editor 디버그 드로잉 여부 |
| `_IndicatorPanelClass` | Indicator Panel Widget Class |
| `_IndicatorPanelZOrder` | Panel Viewport ZOrder |

주요 API:

```cpp
void SetDetectMode(EInteractionDetectMode _detect_mode);
void SetDetectableRange(float _range);
void SetTargetableRange(float _range);
void TryInteract();

EInteractionDetectMode GetDetectMode() const;
AActor* GetTargetedActor() const;
```

주의:

- `TryInteract()`는 현재 코드 기준 `BlueprintCallable`이 아니므로 C++ 입력 처리에서 직접 호출하거나, 프로젝트 Pawn/Character에 BlueprintCallable 래퍼를 만드는 방식으로 사용한다.
- `_DetectableRange`는 `_TargetableRange`보다 작을 수 없고, `_TargetableRange`는 `_DetectableRange`보다 클 수 없다. Setter에서 보정한다.
- `_DetectMode == NA`이면 상태를 초기화하고 Tick을 끈다.

### `FInteractionActorInfo`

파일: `Source/InteractionSystem/Public/InteractorComponent.h`

`UInteractorComponent`가 Overlap 중인 Actor별 상태를 저장하는 내부 구조체다.

| 데이터 | 의미 |
| --- | --- |
| `State` | 마지막으로 통지한 Interaction State |
| `IsDetectedAndVisible` | 현재 프레임에서 감지 및 가시성이 확인되었는지 |
| `OverlapCount` | 여러 Primitive overlap을 고려한 중복 카운트 |

### `UInteractionSystemDeveloperSettings`

파일: `Source/InteractionSystem/Public/InteractionSystemDeveloperSettings.h`

Project Settings에 저장되는 전역 설정이다.

| 설정 | 의미 |
| --- | --- |
| `_OverlayMaterialClass` | Targeted Actor Mesh에 적용할 Overlay Material |

현재 프로젝트 설정 예:

```ini
[/Script/InteractionSystem.InteractionSystemDeveloperSettings]
_OverlayMaterialClass=/InteractionSystem/Materials/MI_OverlayOutline.MI_OverlayOutline
```

### `UUI_InteractionIndicator`

파일: `Source/InteractionSystem/Public/UI_InteractionIndicator.h`

Actor 하나에 대응하는 개별 Indicator Widget이다. `UWidgetBase`를 상속한다.

주요 데이터/API:

| 항목 | 의미 |
| --- | --- |
| `_InteractionActor` | 이 Indicator가 표시하는 Actor |
| `_ActorState` | 현재 표시 상태 |
| `SetDetectedIconSize` | BlueprintImplementableEvent. Indicator 크기 전달 |
| `SetInteractionActor` | Actor와 초기 상태 지정 |
| `ClearWidget` | Actor 참조 제거 및 `None` 상태 처리 |
| `SetActorState` | 상태별 Show/Hide 처리. BlueprintNativeEvent |

기본 상태 처리:

- `None`: Collapsed
- `Detected`: SelfHitTestInvisible로 표시
- `Targeted`: Collapsed  
  Targeted Actor는 개별 아이콘 대신 Panel의 Targeted UI로 표시하는 구조다.

### `UUI_InteractionIndicatorPanel`

파일: `Source/InteractionSystem/Public/UI_InteractionIndicatorPanel.h`

여러 `UUI_InteractionIndicator`를 Pool로 관리하고, 매 Tick Actor 위치를 화면 좌표로 투영해 Canvas 위에 배치하는 Panel Widget이다.

주요 데이터/API:

| 항목 | 의미 |
| --- | --- |
| `CP_Indicators` | Indicator들을 붙일 `CanvasPanel` BindWidget |
| `_IndicatorClass` | Pool에서 생성할 Indicator Widget Class |
| `_IndicatorSize` | Indicator 아이콘 크기 |
| `_IndicatorPoolSize` | 초기 Pool 크기 |
| `_PerspectiveMinScale` | 거리 기반 최소 스케일 |
| `AddInteractionActor` | Actor용 Indicator 활성화 |
| `RemoveInteractionActor` | Actor용 Indicator 비활성화 |
| `SetInteractionActorState` | 특정 Actor Indicator 상태 변경 |
| `SetTargetedActor` | Targeted Panel 표시 대상 갱신 |
| `ShowTargetedPanel` | BlueprintImplementableEvent. Targeted UI 표시/숨김 |

---

## 감지/선택 흐름

### 공통 흐름

1. `UInteractorComponent`가 Overlap으로 후보 Actor를 수집한다.
2. 후보 Actor가 `UInteractableInterface`를 구현하지 않으면 무시한다.
3. 후보가 하나 이상 있고 DetectMode가 `NA`가 아니면 Tick을 켠다.
4. Tick마다 현재 View 위치/방향을 얻는다.
5. DetectMode에 맞게 감지 가능 Actor를 갱신한다.
6. Targeted Actor를 하나 선택한다.
7. Actor별 상태를 `None`, `Detected`, `Targeted`로 갱신한다.
8. Indicator Panel, Overlay Material, `SetInteractionState`를 동기화한다.

### `CameraCenter` 모드

감지 조건:

1. `GetInteractionLocation`이 `DetectableRange` 안에 있음
2. `CanBeDetected()`가 true
3. Visibility trace 결과 visible

Target 선택 조건:

1. 감지/가시 상태인 Actor
2. `TargetableRange` 안에 있음
3. View Forward와 대상 방향의 Dot이 `_MaxViewHalfAngleDegrees` 기준 이상
4. Dot이 가장 큰 Actor를 Targeted Actor로 선택

### `Cursor` 모드

감지 조건:

1. 마우스 위치를 얻을 수 있음
2. `GetInteractionLocation`이 `DetectableRange` 안에 있음
3. `CanBeDetected()`가 true
4. Visibility trace 결과 visible
5. 화면상 `GetInteractionLocation` 투영 위치가 `_CursorDetectRadius` 안에 있음

Target 선택 조건:

1. `GetHitResultUnderCursorForObjects(_OverlapObjectTypes, true, Hit)` 성공
2. Hit Actor가 Overlap 후보이고 감지/가시 상태
3. `TargetableRange` 안에 있음

---

## Visibility / Overlay 정책

### Visibility trace

`IsActorVisible`은 Player View 위치에서 Actor의 `GetInteractionLocation`까지 `ECC_Visibility`로 LineTrace한다.

정책:

- 아무것도 맞지 않으면 visible
- `CanBeCollapsedBySelf()`가 false이면 Hit Actor가 대상 Actor일 때 visible
- `CanBeCollapsedBySelf()`가 true이면 어떤 Visibility Hit가 있어도 visible이 아님

대상 Actor 자신의 Mesh가 InteractionLocation을 가리는 구성이면 `CanBeCollapsedBySelf`를 true로 둘지 신중히 결정한다.

### Overlay Material

Targeted 상태의 Actor에만 Project Settings의 `_OverlayMaterialClass`를 적용한다.

현재 구현 정책:

- Overlay는 로컬 컨트롤러에서만 적용한다.
- Static Mesh의 Overlay Material은 Nanite와 함께 사용할 수 없으므로 적용 중에는 `SetForceDisableNanite(true)`를 호출한다.
- 원래 `ForceDisableNanite` 상태를 저장했다가 Overlay 해제 시 복원한다.
- Overlay 적용 대상은 `GetEffectedMeshComponents()` 반환값으로 결정한다.

---

## UI / Editor 지원

### 기본 제공 콘텐츠

플러그인은 샘플/기본 에셋을 포함한다.

| 경로 | 역할 |
| --- | --- |
| `Content/InteractSampleActor_BP.uasset` | Interactable 샘플 Actor |
| `Content/UI_InteractionIndicator_BP.uasset` | 개별 Indicator 샘플 Widget |
| `Content/UI_InteractionIndicatorPanel_BP.uasset` | Indicator Panel 샘플 Widget |
| `Content/Materials/M_OverlayOutline.uasset` | Overlay 기본 Material |
| `Content/Materials/MI_OverlayOutline.uasset` | Overlay 기본 Material Instance |

### Editor Visualizer

`InteractionSystemEditor` 모듈은 `UInteractorComponent`에 Component Visualizer를 등록한다.

`_ShowDebug`가 true이고 DetectMode가 `NA`가 아니면 에디터에서 다음 정보를 그린다.

- Detectable Range
- Targetable Range
- 현재 View 방향
- Overlap Actor별 Interaction Location
- CameraCenter 모드의 시야 원/콘
- Cursor 모드의 가이드 원

런타임 Development/Editor 빌드에서는 `ToggleDebug()`로 DrawDebug 표시를 토글할 수 있다. Shipping 빌드에서는 Debug 드로잉이 비활성이다.

---

## 제한/주의 사항

- `_OverlapObjectTypes`는 반드시 설정해야 한다.
- `TryInteract()`는 C++ API다. Blueprint 입력에서 바로 호출하려면 래퍼를 추가한다.
- Indicator Panel은 로컬 컨트롤러에서만 생성된다.
- Overlay Material도 로컬 컨트롤러 기준으로만 적용된다.
- `GetInteractionLocation()`이 Actor 내부 깊숙한 위치면 Visibility trace 때문에 감지가 불안정할 수 있다.
- `Cursor` 모드는 마우스 위치와 Hit Result가 필요하므로 PlayerController 입력 모드/커서 설정의 영향을 받는다.
- 여러 시스템이 같은 Mesh의 Overlay Material을 동시에 제어하는 구조가 되면 별도의 참조 카운트/우선순위 관리가 필요하다.

