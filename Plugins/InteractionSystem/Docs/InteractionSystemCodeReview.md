# InteractionSystem 플러그인 코드 체크 / 리뷰 결과

검토 일시: 2026-04-16  
검토 범위: `Plugins/InteractionSystem/Source/InteractionSystem`, `Plugins/InteractionSystem/Source/InteractionSystemEditor`

## 요약

- 런타임 상호작용 로직에서 **충돌 채널 설정이 실제 감지 구독에 반영되지 않는 결함**을 확인했습니다.
- UI 인디케이터 패널은 `BindWidget` 누락 시 **즉시 널 역참조로 크래시**할 수 있습니다.
- 런타임 모듈의 `Build.cs`는 public 헤더가 사용하는 모듈을 private dependency로 선언하고 있어 **외부 모듈 include/빌드 안정성이 낮습니다**.

## 상세 결과

### 1. High — `_CollisionChannel` 설정이 겹침 감지에 적용되지 않음

**근거**
- `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractorComponent.h:30-31`
  - `_CollisionChannel`이 편집 가능한 설정값으로 노출되어 있습니다.
- `Plugins/InteractionSystem/Source/InteractionSystem/Private/InteractorComponent.cpp:477-477`
  - 커서 타게팅은 `_CollisionChannel`을 사용합니다.
- `Plugins/InteractionSystem/Source/InteractionSystem/Private/InteractorComponent.cpp:603-605`
  - 실제 겹침 응답은 `SetCollisionResponseToAllChannels(ECR_Overlap);`만 호출합니다.

**영향**
- 사용자가 선택한 충돌 채널과 무관하게 모든 채널을 overlap 하므로, `_OverlappedActorInfos` 후보군이 과도하게 늘어납니다.
- 커서 타게팅은 `_CollisionChannel`을 따르지만 overlap 수집은 따르지 않아, 감지/타게팅 결과가 서로 어긋날 수 있습니다.

**권장 조치**
- 모든 채널을 ignore로 초기화한 뒤 `_CollisionChannel`에만 `ECR_Overlap`을 적용하도록 수정이 필요합니다.

### 2. High — `CP_Indicators` 바인딩 누락 시 패널에서 널 역참조 가능

**근거**
- `Plugins/InteractionSystem/Source/InteractionSystem/Public/UI_InteractionIndicatorPanel.h:21-22`
  - `CP_Indicators`는 `BindWidget` 의존 필드입니다.
- `Plugins/InteractionSystem/Source/InteractionSystem/Private/UI_InteractionIndicatorPanel.cpp:144-150`
  - `CP_Indicators` 유효성 확인 없이 `AddChildToCanvas`를 호출합니다.

**영향**
- 위젯 블루프린트 리네임/삭제/바인딩 누락 시 런타임에서 즉시 크래시할 수 있습니다.

**권장 조치**
- `AddInteractionActor` 진입 시 `CP_Indicators` 널 가드를 추가하고, 초기화 단계에서 필수 위젯 누락을 로그로 명확히 노출하는 편이 안전합니다.

### 3. Medium — public 헤더가 private dependency 모듈에 의존함

**근거**
- `Plugins/InteractionSystem/Source/InteractionSystem/InteractionSystem.Build.cs:25-45`
  - `PublicDependencyModuleNames`에는 `Core`만 있고, `Engine`, `InputCore`, `CommonLibrary`는 private dependency입니다.
- `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractorComponent.h:6-7`
  - public 헤더가 `Components/SphereComponent.h`를 포함합니다. (`Engine` 필요)
- `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractionSystemDefines.h:5-7`
  - public 헤더가 `InputCoreTypes.h`를 포함합니다. (`InputCore` 필요)
- `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractableInterface.h:7-8`
  - public 헤더가 `CommonUtils.h`를 포함합니다. (`CommonLibrary` 필요)

**영향**
- 현재 모듈을 include 하는 외부 모듈이 우연히 동일 dependency를 이미 가지고 있지 않으면 빌드/인텔리센스가 깨질 수 있습니다.

**권장 조치**
- public 헤더에서 직접 노출하는 타입의 소속 모듈은 `PublicDependencyModuleNames`로 승격하는 편이 맞습니다.

## 참고

- 에디터 모듈(`InteractionSystemEditor`) 자체에서는 즉시 수정이 필요한 치명 결함은 찾지 못했습니다.
- 이번 작업은 **리뷰 결과 보고** 범위로 처리했으며, 코드 수정은 포함하지 않았습니다.
