# InteractionSystem Plugin

- Plugin Dependencies: CommonLibrary, CustomUI
- Engine context: Unreal Engine 5
- Review date: 2026-04-16

## 개요

InteractionSystem 플러그인은 플레이어 주변의 상호작용 가능 액터를 감지하고, 현재 감지/타겟 상태를 인터페이스와 UI 인디케이터에 반영하는 런타임/에디터 보조 플러그인입니다.

구성은 크게 두 부분으로 나뉩니다.

- `InteractionSystem` 런타임 모듈
  - `UInteractorComponent`: 감지 구체, 시야/커서 기반 타게팅, 상호작용 실행 담당
  - `IInteractableInterface`: 상호작용 대상이 구현해야 하는 계약
  - `UUI_InteractionIndicator`, `UUI_InteractionIndicatorPanel`: 월드 액터 상태를 UI로 표시
- `InteractionSystemEditor` 에디터 모듈
  - `FInteractorComponentVisualizer`: 디버그 시야각/반경 시각화

## 현재 동작 흐름

1. `UInteractorComponent`가 겹침 액터를 추적합니다.
2. 감지 모드(`CameraCenter`, `Cursor`)에 따라 현재 보이고 선택 가능한 액터를 계산합니다.
3. `IInteractableInterface::SetInteractionState`로 대상 상태를 전달합니다.
4. `UUI_InteractionIndicatorPanel`이 액터별 인디케이터와 타겟 패널을 갱신합니다.

## 코드 리뷰 결과

### 강점

- 런타임 모듈과 에디터 모듈이 분리되어 있어 UE5 플러그인 구조가 비교적 명확합니다.
- `IInteractableInterface` 기반으로 상호작용 대상을 느슨하게 연결해 재사용성이 좋습니다.
- 인디케이터 패널이 풀링 구조를 사용해 위젯 재생성을 줄이려는 의도가 보입니다.

### 주요 우려 사항

1. **공개 헤더와 Build.cs의 의존성 경계가 맞지 않습니다.**
   - 위치:
     - `Plugins/InteractionSystem/Source/InteractionSystem/InteractionSystem.Build.cs:25-45`
     - `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractorComponent.h:5-7`
     - `Plugins/InteractionSystem/Source/InteractionSystem/Public/UI_InteractionIndicator.h:5-6`
   - 내용:
     - 공개 헤더가 `USphereComponent`, `UWidgetBase` 등 Engine/CustomUI 타입을 직접 노출하지만, 해당 모듈들이 `PrivateDependencyModuleNames`에만 들어 있습니다.
   - 영향:
     - 다른 모듈이 `InteractionSystem` 공개 헤더를 포함할 때 전이 의존성에 기대게 되어, 모듈 경계가 깨지거나 빌드 설정 변경 시 쉽게 깨질 수 있습니다.

2. **`_CollisionChannel` 설정이 감지 구체(Overlap) 설정에 반영되지 않습니다.**
   - 위치:
     - `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractorComponent.h:30-31`
     - `Plugins/InteractionSystem/Source/InteractionSystem/Private/InteractorComponent.cpp:476-477`
     - `Plugins/InteractionSystem/Source/InteractionSystem/Private/InteractorComponent.cpp:603-606`
   - 내용:
     - 에디터에서 지정하는 `_CollisionChannel`은 커서 타겟팅의 `GetHitResultUnderCursor()`에만 사용되고, 실제 감지용 SphereComponent는 `SetCollisionResponseToAllChannels(ECR_Overlap)`로 모든 채널을 겹침 처리합니다.
   - 영향:
     - 설정 이름과 실제 동작이 어긋나므로, 디자이너가 충돌 채널을 조정해도 감지 범위 필터링에는 영향을 주지 않습니다.
     - 불필요한 overlap 이벤트가 늘어나 유지보수성과 성능 추론이 어려워집니다.

3. **런타임에 감지/타게팅 거리 변경 시 인디케이터 패널 스케일 기준이 동기화되지 않습니다.**
   - 위치:
     - `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractorComponent.h:82-86`
     - `Plugins/InteractionSystem/Source/InteractionSystem/Private/InteractorComponent.cpp:198-231`
     - `Plugins/InteractionSystem/Source/InteractionSystem/Private/InteractorComponent.cpp:621-622`
   - 내용:
     - `SetDetectableRange()`, `SetTargetableRange()`는 내부 거리 값과 Sphere 반경만 갱신하고, `_IndicatorPanel->SetPerspectiveDistance()`는 패널 생성 시 1회만 호출됩니다.
   - 영향:
     - 런타임 튜닝이나 블루프린트 조정 이후 UI 인디케이터 크기/원근감 기준이 실제 상호작용 거리와 어긋날 수 있습니다.

### 보조 메모

- `Plugins/InteractionSystem/InteractionSystem.uplugin:5-12`의 메타데이터가 비어 있어 플러그인 브라우저/외부 공유 시 용도 파악이 어렵습니다.
- 이 README는 위 공백을 보완하기 위한 첫 문서화 결과물입니다.

## 권장 후속 작업

1. `InteractionSystem.Build.cs`의 public/private dependency 경계를 공개 헤더 기준으로 다시 정리합니다.
2. `_CollisionChannel`이 실제 overlap 필터에도 반영되도록 `ApplyCollisionChannelSettings()`의 의도를 명확히 맞춥니다.
3. 런타임 거리 변경 API가 인디케이터 패널 원근 설정까지 갱신하도록 동기화합니다.

## 검토 범위

- `Plugins/InteractionSystem/InteractionSystem.uplugin`
- `Plugins/InteractionSystem/Source/InteractionSystem/**`
- `Plugins/InteractionSystem/Source/InteractionSystemEditor/**`
