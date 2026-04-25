# StoryFlow 플러그인 정리

이 문서는 `Plugins/StoryFlow` 플러그인의 **현재 구현 상태**를 빠르게 파악하기 위한 문서다.  
설명 기준은 **지금 코드에 반영된 동작**이며, 오래된 설계 메모가 아니라 실제 구현 기준으로 정리했다.

---

# 1. 플러그인 목적

`StoryFlow`는 **Scene(장소 단위)** 안에서 **Shot(진행 단계 단위)** 흐름을 그래프로 편집하고,  
런타임에서는 이를 순차 실행하는 플러그인이다.

핵심 목표:

- `StorySceneAsset` 하나로 장소 단위 씬 구성
- 각 Scene은 자기 `TargetLevel`을 가짐
- Scene 시작 시 `SceneTemplate`가 먼저 진입
- 그 다음 EntryShot 또는 지정 Shot부터 실행
- 현재 진행 상태를 `SceneID + ShotID`로 저장/복원 가능

---

# 2. 큰 구조

## 런타임 개념

- **Scene**
  - 하나의 장소
  - `TargetLevel`과 `SceneTemplate`를 가짐

- **Shot**
  - Scene 안의 실제 진행 단계
  - `UStoryShotBase` 파생 로직이 담당

- **Subsystem**
  - Scene 시작 요청
  - 레벨 전환/로딩
  - SceneTemplate 실행
  - Shot 실행/전환

## 에디터 개념

- `StorySceneAsset`
  - 전용 에디터에서 그래프 편집

- `Entry` 노드
  - 시작 Shot 결정

- `Shot` 노드
  - 각 노드가 `UStorySceneNodeData` 하나에 대응

---

# 3. Runtime 구조

## 3.1 `FStorySceneID`
파일: `Source/StoryFlow/Public/StoryFlowDefines.h`

- Scene 식별자
- 내부 값은 `FName`

---

## 3.2 `FStoryShotID`
파일: `Source/StoryFlow/Public/StoryFlowDefines.h`

- Shot 식별자
- 내부 값은 `FName`

---

## 3.3 `FStoryFlowRef`
파일: `Source/StoryFlow/Public/StoryFlowDefines.h`

- 현재 진행 상태를 나타내는 최소 구조
- 포함 값:
  - `SceneID`
  - `ShotID`

저장/복원은 이 구조 기준으로 한다.

---

## 3.4 `UStorySceneBase`
파일:
- `Source/StoryFlow/Public/StorySceneBase.h`
- `Source/StoryFlow/Private/StorySceneBase.cpp`

Scene 단위 실행 로직 베이스 클래스.

역할:

- Scene 시작 시 1회 실행되는 로직
- Scene 종료 시 정리 로직

주요 함수:

- `InitializeScene(const FStoryFlowRef&)`
- `EnterScene()`
- `ExitScene()`
- `OnEnterScene`
- `OnExitScene`

중요:

- `StorySceneAsset`에 붙어 있는 Template 원본을 직접 실행하지 않음
- 런타임에서는 `DuplicateObject(..., Subsystem)`로 인스턴스를 생성해서 실행

---

## 3.5 `UStoryShotBase`
파일:
- `Source/StoryFlow/Public/StoryShotBase.h`
- `Source/StoryFlow/Private/StoryShotBase.cpp`

Shot 단위 실행 로직 베이스 클래스.

역할:

- Shot 진입
- Tick
- 완료 판정
- 종료

주요 함수:

- `InitializeShot()`
- `EnterShot()`
- `TickShot()`
- `ExitShot()`
- `FinishShot()`
- `OnEnterShot`
- `OnTickShot`
- `OnExitShot`

---

## 3.6 `UStorySceneNodeData`
파일:
- `Source/StoryFlow/Public/StorySceneNodeData.h`
- `Source/StoryFlow/Private/StorySceneNodeData.cpp`

Shot 노드 1개에 대응하는 데이터 오브젝트.

포함 값:

- `_ShotID`
- `_DisplayName`
- `_Description`
- `_ShotTemplate`
- `_NextShotIDs`

책임:

- 노드 메타데이터
- Shot 템플릿 참조
- 다음 Shot 연결 정보

---

## 3.7 `UStorySceneAsset`
파일:
- `Source/StoryFlow/Public/StorySceneAsset.h`
- `Source/StoryFlow/Private/StorySceneAsset.cpp`

Scene 전체를 나타내는 에셋.

포함 값:

- `_SceneID`
- `_DisplayName`
- `_TargetLevel`
- `_EntryShotID`
- `_SceneTemplate`
- `_ShotNodes`
- `_EditorGraph` (에디터 전용)

의미:

- `TargetLevel`
  - 이 Scene이 속한 레벨
- `SceneTemplate`
  - Scene 단위 실행 로직
- `_ShotNodes`
  - Scene 내부 Shot 목록

---

## 3.8 `UStorySceneRegistryAsset`
파일:
- `Source/StoryFlow/Public/StorySceneRegistryAsset.h`
- `Source/StoryFlow/Private/StorySceneRegistryAsset.cpp`

Scene 목록 레지스트리.

역할:

- `SceneID -> SceneAsset` 조회

구성:

- `_Scenes`
  - `FStorySceneReference` 배열

---

## 3.9 `UStoryFlowDeveloperSettings`
파일:
- `Source/StoryFlow/Public/StoryFlowDeveloperSettings.h`
- `Source/StoryFlow/Private/StoryFlowDeveloperSettings.cpp`

프로젝트 전역 설정.

현재 값:

- `_StorySceneRegistry`
- `_LoadingLevel`
- `_MinimumLoadingLevelDuration`

의미:

- `LoadingLevel`
  - 다른 레벨 Scene 시작 시 먼저 보여줄 로딩 맵
- `MinimumLoadingLevelDuration`
  - LoadingLevel 최소 체류 시간

---

## 3.10 `UStoryFlowSubsystem`
파일:
- `Source/StoryFlow/Public/StoryFlowSubsystem.h`
- `Source/StoryFlow/Private/StoryFlowSubsystem.cpp`

StoryFlow 전체 런타임 관리자.

현재 상태:

- `_CurrentSceneAsset`
- `_CurrentSceneInstance`
- `_CurrentShotNode`
- `_CurrentShotInstance`

Pending 상태:

- `_PendingStartRef`
- `_PendingTargetLevel`
- `_PendingTravelPhase`
- `_PendingTargetLevelLoadHandle`
- `_PendingTargetLevelLoadCompleted`
- `_PendingLoadingLevelEnterTime`

주요 공개 함수:

- `StartFromScene()`
- `StartFromRef()`
- `StopScene()`
- `GetCurrentSceneAsset()`
- `GetCurrentScene()`
- `GetCurrentShot()`
- `GetCurrentRef()`

---

# 4. 레벨 전환 / Scene 시작 흐름

## 4.1 같은 레벨이면

현재 레벨 == `SceneAsset.TargetLevel`

1. `StartFromScene` / `StartFromRef`
2. SceneAsset 조회
3. `StartResolvedScene()`
4. `SceneTemplate.EnterScene()`
5. EntryShot 또는 지정 Shot 시작

---

## 4.2 다른 레벨이면

현재 레벨 != `SceneAsset.TargetLevel`

### `LoadingLevel`이 있으면

1. pending start 정보 저장
2. `LoadingLevel` 오픈
3. `PostLoadMapWithWorld`에서 LoadingLevel 진입 확인
4. `TargetLevel` 비동기 로드 시작
5. 로딩 퍼센트 추적
6. async load 완료 + 최소 체류시간 만족 시 `OpenLevel(TargetLevel)`
7. `PostLoadMapWithWorld`에서 TargetLevel 진입 확인
8. `StartResolvedScene()`
9. `SceneTemplate.EnterScene()`
10. Shot 시작

### `LoadingLevel`이 없으면

1. `TargetLevel` 비동기 로드 시작
2. async load 완료 시 바로 `OpenLevel(TargetLevel)`
3. TargetLevel 진입 후 Scene 시작

---

# 5. Shot 실행 흐름

1. `MoveToShot(ShotID)`
2. `SceneAsset.FindShotNode()`
3. `ShotTemplate` 찾기
4. `DuplicateObject<UStoryShotBase>(template, this)`
5. `InitializeShot(ShotID)`
6. `EnterShot()`
7. Tick에서 `TickShot()`
8. 완료되면 `NextShotIDs[0]`로 이동

중요:

- 현재는 **선형 흐름만 지원**
- `NextShotIDs`가 배열이어도 실제 런타임은 첫 번째만 사용

---

# 6. 에디터 구조

## 6.1 `FStoryFlowEditorModule`
파일:
- `Source/StoryFlowEditor/Public/StoryFlowEditorModule.h`
- `Source/StoryFlowEditor/Private/StoryFlowEditor.cpp`

역할:

- Story Scene 에셋 타입 등록
- 노드 커스텀 UI 등록
- PIE authorizer 등록

---

## 6.2 `FStorySceneAssetTypeActions`
파일:
- `Source/StoryFlowEditor/Public/StorySceneAssetTypeActions.h`
- `Source/StoryFlowEditor/Private/StorySceneAssetTypeActions.cpp`

`StorySceneAsset` 더블클릭 시 전용 에디터를 연다.

---

## 6.3 `UStorySceneFactory`
파일:
- `Source/StoryFlowEditor/Public/StorySceneFactory.h`
- `Source/StoryFlowEditor/Private/StorySceneFactory.cpp`

새 `StorySceneAsset` 생성 팩토리.

---

## 6.4 `FStorySceneEditor`
파일:
- `Source/StoryFlowEditor/Private/StorySceneEditor.h`
- `Source/StoryFlowEditor/Private/StorySceneEditor.cpp`

Story Scene 전용 에디터.

구성:

- Graph 탭
- Details 탭
- Compile 버튼

기능:

- 그래프 생성/열기
- Details 편집
- Compile
- Delete / Undo / Redo
- PIE 전 compile 검증

---

## 6.5 `UStorySceneEdGraph`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneEdGraph.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneEdGraph.cpp`

역할:

- Entry 노드 보장
- 그래프를 런타임 데이터로 재구성 (`RebuildRuntimeData`)

중요:

- **런타임 데이터 동기화 책임은 GraphSchema가 아니라 `RebuildRuntimeData()`**

---

## 6.6 `UStorySceneGraphNode_Entry`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Entry.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Entry.cpp`

특징:

- 삭제 불가
- 복제 불가
- `Next` 출력 핀 1개

---

## 6.7 `UStorySceneGraphNode_Shot`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Shot.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Shot.cpp`

역할:

- 생성 시 `UStorySceneNodeData` 생성
- 삭제 시 대응 NodeData 제거
- 노드 제목 표시

현재 추가된 편의 기능:

- 노드 바깥 우측 상단 Play 버튼
- 버튼 클릭 시 해당 Shot부터 PIE 시작
- PIE 중 버튼 숨김

Desc 표시:

- 툴팁이 아니라 **comment bubble**
- Compile 시 `NodeComment`로 동기화

---

## 6.8 `UStorySceneGraphSchema`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphSchema.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphSchema.cpp`

그래프 연결 규칙 담당.

현재 규칙:

- Output 1개 연결
- Input 1개 연결
- Entry → Shot
- Shot → Shot

즉 현재 구조는 **직선형 흐름** 기준이다.

---

# 7. Compile 동작

`StorySceneEditor`의 Compile 버튼은 다음 순서로 동작한다.

1. ShotID 자동 보정
2. Desc를 comment bubble용 `NodeComment`로 동기화
3. 그래프 기준 런타임 데이터 재생성
   - `EntryShotID`
   - `NextShotIDs`
4. 무결성 검사

---

# 8. Compile 무결성 검사

현재 검사 항목:

## Scene 수준

- `SceneID`가 비어 있지 않은가
- `TargetLevel`이 설정되어 있는가
- `SceneTemplate`이 설정되어 있는가
- `EntryShotID`가 유효한가
- `EntryShotID`가 실제 ShotNodes 안에 존재하는가

## Shot 수준

- `ShotID`가 비어 있지 않은가
- `ShotID`가 중복되지 않는가
- `ShotTemplate`이 설정되어 있는가
- `NextShotIDs`에 빈 ShotID가 없는가
- `NextShotIDs`가 실제 존재하는 Shot을 가리키는가

오류가 있으면:

- Compile 실패
- 노드 하단 빨간 오류 메시지
- PIE 시작 차단

---

# 9. 로딩 관련 현재 상태

현재 구현됨:

- `LoadingLevel` 진입
- `TargetLevel` async load
- async load progress 추적
- `TRACE_LOG`로 10% 단위 퍼센트 출력
- `MinimumLoadingLevelDuration` 충족 전까지 TargetLevel 오픈 지연

현재 미구현:

- 로딩 퍼센트 UI 위젯 연동
- 최소 체류시간 완료 전용 연출
- 세밀한 progress bar

---

# 10. 저장 / 복원

저장 최소 단위:

- `SceneID`
- `ShotID`

복원:

- `StartFromRef(FStoryFlowRef)`
- 같은 레벨이면 바로 시작
- 다른 레벨이면 레벨 전환 후 시작

---

# 11. 현재 장점

- Scene / Shot 구조가 분리됨
- SceneTemplate / ShotTemplate 패턴이 일관됨
- 레벨 준비 후 Scene 시작 구조 확보
- 저장/복원이 `SceneID + ShotID`로 단순함
- 에디터 편집과 런타임 실행이 분리됨

---

# 12. 현재 한계

- 분기 실행 미지원
  - `NextShotIDs[0]`만 사용
- 로딩 UI 위젯 미연동
- 스폰 위치/시작 포인트 시스템 미구현
- SceneTemplate와 Level 진입 연출의 세분화는 아직 최소 버전

---

# 13. 추천 코드 읽기 순서

1. `StoryFlowDefines.h`
2. `StorySceneBase.h`
3. `StoryShotBase.h`
4. `StorySceneNodeData.h`
5. `StorySceneAsset.h`
6. `StoryFlowDeveloperSettings.h`
7. `StoryFlowSubsystem.h/.cpp`
8. `StorySceneEditor.cpp`
9. `StorySceneEdGraph.cpp`
10. `StorySceneGraphNode_Shot.cpp`

---

# 14. 한 문장 요약

`StoryFlow`는

> **Scene(장소) 단위 에셋에 TargetLevel/SceneTemplate/Shot 흐름을 정의하고, 필요 시 LoadingLevel을 거쳐 TargetLevel을 비동기 준비한 뒤 SceneTemplate와 Shot을 순차 실행하는 스토리 흐름 플러그인**이다.
