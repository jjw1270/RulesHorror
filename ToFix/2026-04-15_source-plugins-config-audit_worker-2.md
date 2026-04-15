# Source / Plugins / Config 감사 메모 (worker-2)

작성일: 2026-04-15
대상: `Source/`, `Plugins/`, `RulesHorror.uproject`, 관련 런타임 코드

## 요약
- **빌드 관점 결론**: `RulesHorrorEditor Win64 Development` 전체 빌드는 성공했다. 즉, 이번 범위에서 **Source / Plugins / Config 자체가 즉시 빌드를 막는 치명적 오구성은 확인되지 않았다.**
- **실제 ToFix 우선순위**는 빌드 설정 자체보다, 그 위에 얹힌 **로직 누락 / 오프바이원 / 에디터 초기화 비용 / 테스트 부재** 쪽에 있다.

## 확인한 증거
1. 전체 에디터 빌드 성공
   - 명령: `F:\UnrealEngine\UE_5.7\Engine\Build\BatchFiles\Build.bat RulesHorrorEditor Win64 Development F:\UnrealProjects\RulesHorror\RulesHorror.uproject -WaitMutex -NoHotReloadFromIDE`
   - 결과: `Result: Succeeded`
2. 자동화 테스트 코드 검색
   - 명령: `rg -n 'IMPLEMENT_SIMPLE_AUTOMATION_TEST|IMPLEMENT_COMPLEX_AUTOMATION_TEST|BEGIN_DEFINE_SPEC|DEFINE_SPEC|AutomationTest' Source Plugins`
   - 결과: `NO_AUTOMATION_TESTS_FOUND`
3. 린트 설정 검색
   - 확인 파일: `.clang-format`, `.clang-tidy`, `.editorconfig`
   - 결과: `NO_LINT_CONFIG_FOUND`

## 주요 문제 목록

### 1) 스토리 진행상황 복원이 실제로 구현되어 있지 않음
- 심각도: **중간 (기능 누락)**
- 증거:
  - `Source/RulesHorror/UI/Lobby/MonitorScreen/Explorer/StoryListSite/Site_StoryList.cpp:46-50`
  - `USaveGameHelper::GetSaveGame(this)` 호출 직후 `// TODO : Story 진행상황 불러오기`만 존재
- 영향:
  - 저장 데이터가 있어도 스토리 목록 UI가 진행 상태를 복원하지 못한다.
  - SaveGame 시스템을 이미 참조하고 있으므로, 사용자 기대치와 실제 동작이 어긋날 가능성이 높다.
- 개선안:
  - SaveGame에 스토리 해금/읽음/진행 인덱스를 명시적으로 보관하고, `NativeConstruct()`에서 UI 반영까지 완료해야 한다.
  - TODO가 남아 있는 동안은 관련 버튼/상태 표현이 완전하다고 가정하면 안 된다.

### 2) 스토리 리스트 페이지 수 계산이 오프바이원이라 마지막 빈 페이지가 생길 수 있음
- 심각도: **중간 (로직 오류)**
- 증거:
  - `Source/RulesHorror/UI/Lobby/MonitorScreen/Explorer/StoryListSite/Site_StoryList.cpp:61-65`
  - 현재 코드: `const int32 max_radio_button_idx = (all_story_item_num / _StoryTitleNum) + 1;`
- 영향:
  - 스토리 개수가 페이지 크기의 배수일 때도 페이지를 1개 더 만든다.
  - 예: 스토리 10개, 페이지당 5개면 실제는 2페이지인데 현재 식은 3페이지를 만든다.
  - 결과적으로 마지막 페이지가 비어 보이거나 UI 상태가 어색해질 수 있다.
- 개선안:
  - 정수 올림 나눗셈으로 바꿔야 한다. 예: `(all_story_item_num + _StoryTitleNum - 1) / _StoryTitleNum`
  - 최소 페이지 수 1 보장이 필요하면 별도로 `FMath::Max(1, ...)` 처리.

### 3) 아이템 레지스트리 갱신이 초기화 시점에 동기 스캔 + 에셋 로드를 즉시 수행함
- 심각도: **중간 (성능 / 메모리 낭비)**
- 증거:
  - `Plugins/ItemCorePlugin/Source/ItemCore/Private/ItemRegistrySubsystem.cpp:15-18`
    - 에디터 초기화 시 `RefreshRegistry()` 즉시 호출
  - `Plugins/ItemCorePlugin/Source/ItemCore/Private/ItemRegistrySubsystem.cpp:42`
    - `asset_registry.ScanPathsSynchronous(...)`
  - `Plugins/ItemCorePlugin/Source/ItemCore/Private/ItemRegistrySubsystem.cpp:55,60`
    - `GetAssets(...)` 후 각 `asset_data.GetAsset()`로 실제 `UDataTable` 로드
  - `Plugins/ItemCorePlugin/Source/ItemCore/Private/ItemRegistryValidationSubsystem.cpp:18`
    - 별도 Validation Subsystem에서도 다시 `RefreshRegistry()` 호출
- 영향:
  - 프로젝트 규모가 커질수록 에디터 진입/리프레시 시 hitch가 커질 수 있다.
  - 현재 구조는 “검색” 단계와 “실제 로드/인덱싱” 단계가 강하게 결합되어 있어, 필요 이상으로 메모리와 시간을 사용한다.
- 개선안:
  - Asset Registry 메타데이터 기반 1차 필터링과 실제 로드를 분리한다.
  - 초기화 시 전체 동기 리프레시 대신, 명시적 갱신/지연 로드/변경된 테이블만 재인덱싱하는 방향을 검토한다.
  - 대규모 데이터로 갈수록 병목 가능성이 커서 선제 정리가 필요하다.

### 4) 자동화 테스트가 사실상 없어서 위 문제들이 회귀하기 쉬움
- 심각도: **중간 (품질 보증 공백)**
- 증거:
  - 자동화 테스트 패턴 검색 결과: `NO_AUTOMATION_TESTS_FOUND`
- 영향:
  - 스토리 리스트 pagination, SaveGame 복원, ItemRegistry 갱신 같은 핵심 로직이 수동 확인에만 의존한다.
  - 작은 수정도 UI/데이터 회귀를 조기에 잡기 어렵다.
- 개선안:
  - 최소한 다음 3가지는 자동화 대상으로 잡는 것이 좋다.
    1. `Site_StoryList` 페이지 수 계산
    2. SaveGame 기반 스토리 진행상황 반영
    3. ItemRegistry 중복 ID / 잘못된 RowStruct 검증

### 5) 복잡도 집중 파일이 커서 향후 수정 위험이 높음
- 심각도: **낮음~중간 (가독성 / 유지보수성)**
- 증거:
  - `Plugins/InteractionSystem/Source/InteractionSystem/Private/InteractorComponent.cpp` — **716 lines**
  - `Source/RulesHorror/Lobby/LobbyPawn.cpp` — **423 lines**
  - `Plugins/CustomUIPlugin/Source/CustomUI/Private/WidgetSubsystem.cpp` — **420 lines**
  - 특히 `InteractorComponent.cpp`는 overlap 처리, 타겟 선정, 시야 판정, 스크린 투영, UI indicator lifecycle, 디버그 드로잉까지 한 파일/클래스에 몰려 있다.
- 영향:
  - 작은 수정도 상호작용 범위가 넓어져 회귀 위험이 커진다.
  - 새 기능 추가 시 책임 경계가 더 흐려질 가능성이 높다.
- 개선안:
  - 계산 로직(타겟 선정/가시성)과 UI 표현(Indicator panel), 디버그/에디터 보조 코드를 분리하는 리팩터링 후보로 관리한다.
  - 지금 당장 대수술보다는, 다음 수정부터 기능 단위로 천천히 분리하는 쪽이 안전하다.

## 최종 판단
- **Source / Plugins / Config 레벨의 치명적 빌드 문제는 이번 점검에서 재현되지 않았다.**
- 다만 실제 사용자/개발 경험을 해치는 문제는 이미 존재한다:
  1. 스토리 진행상황 복원 미구현
  2. 스토리 목록 페이지 계산 오프바이원
  3. ItemRegistry 초기화 비용 구조
  4. 자동화 테스트 부재
- 우선순위는 **(1) 기능 누락/오프바이원 수정 -> (2) 회귀 테스트 추가 -> (3) ItemRegistry 초기화 구조 개선** 순서가 적절하다.

