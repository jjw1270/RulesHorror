# StoryFlow 플러그인 정리

이 문서는 `Plugins/StoryFlow` 플러그인의 **현재 코드 기준 구현 상태**를 정리한 문서다.  
설계 아이디어가 아니라, **지금 실제로 동작하는 구조**를 기준으로 적었다.

---

# 1. 플러그인 목적

`StoryFlow`는 **Scene(장소 단위)** 안에서 **Shot(진행 단계 단위)** 흐름을 그래프로 편집하고,  
런타임에서는 이를 순차 실행하거나 **Branch 분기 / Scene 전이 / 레벨 이동**까지 처리하는 플러그인이다.

핵심 목표:

- `StorySceneAsset` 하나로 장소 단위 Scene 구성
- 각 Scene은 자기 `TargetLevel`을 가짐
- Scene 시작 시 `SceneTemplate`가 먼저 진입
- 그 다음 EntryShot 또는 지정 Shot부터 실행
- Shot의 다음은 다른 Shot / Branch / 다음 Scene(Transition)으로 연결 가능
- Branch 노드에서 여러 갈래 중 다음 진행 경로 선택 가능
- 현재 진행 상태를 `SceneID + ShotID`로 저장/복원 가능

---

# 2. 큰 구조

## 런타임 개념

- **Scene**
  - 하나의 장소
  - `TargetLevel`, `SceneTemplate`를 가짐

- **Shot**
  - Scene 안의 실제 진행 단계
  - `UStoryShotBase` 파생 로직이 담당

- **Branch**
  - 분기 판단 전용 실행 단위
  - `UStoryBranchBase` 파생 로직이 다음 인덱스를 선택

- **Transition**
  - 별도 런타임 실행 객체는 아님
  - 에디터 그래프에서 다음 Scene을 지정하는 종착 노드
  - Compile 시 `NextSceneID` 링크로 접혀 들어감

- **Subsystem**
  - Scene 시작 요청
  - 레벨 전환 / 로딩
  - SceneTemplate 실행
  - Shot 실행 / Branch 평가 / Scene 전이 관리

## 에디터 개념

- `StorySceneAsset`
  - 전용 에디터에서 그래프 편집

- `Entry` 노드
  - 시작 Shot 결정

- `Shot` 노드
  - 실제 실행 가능한 노드
  - `UStorySceneNodeData`와 연결됨

- `Branch` 노드
  - 다중 분기 전용 노드
  - `UStoryBranchNodeData`와 연결됨

- `Transition` 노드
  - 다음 Scene 전이용 노드
  - `NextSceneID`를 가짐

---

# 3. Runtime 구조

## 3.1 `FStorySceneID`
파일: `Source/StoryFlow/Public/StoryFlowDefines.h`

- Scene 식별자
- 내부 값은 `FName`
- 에디터에서는 property customization이 적용되어 `ID` 하위 depth 없이 한 줄로 편집 가능

---

## 3.2 `FStoryShotID`
파일: `Source/StoryFlow/Public/StoryFlowDefines.h`

- Shot 식별자
- 내부 값은 `FName`
- 에디터에서는 property customization이 적용되어 `ID` 하위 depth 없이 한 줄로 편집 가능

---

## 3.3 `FStoryBranchID`
파일: `Source/StoryFlow/Public/StoryFlowDefines.h`

- Branch 식별자
- 내부 값은 `FName`
- 에디터에서는 property customization이 적용되어 `ID` 하위 depth 없이 한 줄로 편집 가능

---

## 3.4 `FStoryFlowRef`
파일: `Source/StoryFlow/Public/StoryFlowDefines.h`

- 현재 진행 상태를 나타내는 최소 구조
- 포함 값:
  - `SceneID`
  - `ShotID`

저장/복원은 이 구조 기준으로 한다.

중요:

- 저장 최소 단위는 `SceneID + ShotID`
- 저장 대상은 **현재 진행 중인 Shot**
- Branch는 실행 중간 판단 단계이므로 별도 저장 키를 두지 않는다
- 따라서 Branch 상태에서는 저장하지 않는 것이 맞다

---

## 3.5 `UStorySceneBase`
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

---

## 3.6 `UStoryShotBase`
파일:
- `Source/StoryFlow/Public/StoryShotBase.h`
- `Source/StoryFlow/Private/StoryShotBase.cpp`

Shot 단위 실행 로직 베이스 클래스.

역할:

- Shot 진입
- Tick
- 완료 판정
- 종료
- 단일 다음 링크 실행

주요 함수:

- `InitializeShot()`
- `EnterShot()`
- `TickShot()`
- `ExitShot()`
- `FinishShot()`
- `OnEnterShot`
- `OnTickShot`
- `OnExitShot`

중요:

- `ExitShot()` 후 다음 링크 판정이 이어진다
- Shot은 현재 런타임 자료구조상 단일 `NextLink`만 가진다
- 따라서 **Shot에서 분기 판단은 하지 않고**, 다중 갈래 선택은 반드시 `Branch` 노드를 통해 수행한다

---

## 3.7 `UStoryBranchBase`
파일:
- `Source/StoryFlow/Public/StoryBranchBase.h`
- `Source/StoryFlow/Private/StoryBranchBase.cpp`

Branch 단위 실행 로직 베이스 클래스.

역할:

- 분기 입력(`FStoryFlowRef`) 수신
- 다음 갈래 인덱스 선택

주요 함수:

- `InitializeBranch(const FStoryFlowRef&)`
- `SelectNextIndex(int32 NextCount)`

중요:

- 분기 선택 책임은 Shot이 아니라 Branch가 가진다
- `BranchCount`가 1이면 사실상 고정 흐름 노드처럼 동작한다

---

## 3.8 `FStorySceneBranchLink`
파일: `Source/StoryFlow/Public/StorySceneNodeData.h`

다음 연결 1개를 표현하는 구조.

포함 값:

- `NextShotID`
- `NextBranchID`
- `NextSceneID`

규칙:

- 셋 중 하나만 유효한 상태를 기대한다
- `NextShotID`면 다음 Shot 이동
- `NextBranchID`면 Branch 평가 단계로 이동
- `NextSceneID`면 다음 Scene으로 전이

---

## 3.9 `UStorySceneNodeData`
파일:
- `Source/StoryFlow/Public/StorySceneNodeData.h`
- `Source/StoryFlow/Private/StorySceneNodeData.cpp`

Shot 노드 1개에 대응하는 데이터 오브젝트.

포함 값:

- `_ShotID`
- `_DisplayName`
- `_Description`
- `_ShotTemplate`
- `_NextLink`

책임:

- Shot 노드 메타데이터
- Shot 템플릿 참조
- 다음 링크 정보 보관

중요:

- 현재 코드상 Shot 쪽에는 별도 `BranchCount`가 없다
- Compile 시 그래프 연결 기준으로 `_NextLink`가 다시 생성된다
- Shot은 다음 링크를 **최대 1개만** 가진다

---

## 3.10 `UStoryBranchNodeData`
파일:
- `Source/StoryFlow/Public/StoryBranchNodeData.h`
- `Source/StoryFlow/Private/StoryBranchNodeData.cpp`

Branch 노드 1개에 대응하는 데이터 오브젝트.

포함 값:

- `_BranchID`
- `_DisplayName`
- `_Description`
- `_BranchTemplate`
- `_BranchCount`
- `_NextLinksByPinIndex`

책임:

- Branch 노드 메타데이터
- Branch 템플릿 참조
- 출력 갈래 수 유지
- 각 갈래의 다음 링크 정보 보관

중요:

- `_BranchCount` 기본값은 `1`이며, 최대값은 `9`로 제한된다
- Details에서 `_BranchCount`를 바꾸면 그래프 출력 핀 `Next_0`, `Next_1` ... 수가 동기화된다
- `_NextLinksByPinIndex`는 Compile 시 그래프 연결을 다시 읽어 재구성된다

---

## 3.11 `UStorySceneAsset`
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
- `_BranchNodes`
- `_EditorGraph` (에디터 전용)

의미:

- `TargetLevel`
  - 이 Scene이 속한 레벨
- `SceneTemplate`
  - Scene 단위 실행 로직
- `ShotNodes`
  - Scene 내부 Shot 목록
- `BranchNodes`
  - Scene 내부 Branch 목록

주요 조회 함수:

- `FindShotNode(FStoryShotID)`
- `FindBranchNode(FStoryBranchID)`

에디터 전용 주요 함수:

- `SetEditorGraph()`
- `SetSceneID()`
- `SetEntryShotID()`
- `CreateShotNode()` / `CreateBranchNode()`
- `AddShotNode()` / `AddBranchNode()`
- `RemoveShotNode()` / `RemoveBranchNode()`

---

## 3.12 `UStorySceneRegistryAsset`
파일:
- `Source/StoryFlow/Public/StorySceneRegistryAsset.h`
- `Source/StoryFlow/Private/StorySceneRegistryAsset.cpp`

`SceneID -> SceneAsset` 조회에 사용하는 전역 레지스트리.

역할:

- `SceneID`로 SceneAsset 찾기
- 다음 Scene 전이 시 대상 SceneAsset resolve
- 에디터 compile 검증 시 SceneID 등록 여부 확인

---

## 3.13 `UStoryFlowDeveloperSettings`
파일:
- `Source/StoryFlow/Public/StoryFlowDeveloperSettings.h`
- `Source/StoryFlow/Private/StoryFlowDeveloperSettings.cpp`

전역 설정.

포함 값:

- `_StorySceneRegistry`
- `_LoadingLevel`
- `_MinimumLoadingLevelDuration`
- `_MinimumLoadingLevelProgressCurve`

---

## 3.14 `UStoryFlowSubsystem`
파일:
- `Source/StoryFlow/Public/StoryFlowSubsystem.h`
- `Source/StoryFlow/Private/StoryFlowSubsystem.cpp`

런타임 전체 흐름을 관리하는 중심 Subsystem.

주요 책임:

- Scene 시작 요청 처리
- SceneID로 SceneAsset 찾기
- 레벨 이동 필요 여부 판단
- LoadingLevel 진입
- TargetLevel async load
- SceneTemplate 진입
- Shot 실행
- Shot 종료 후 다음 링크 처리
- Branch 평가
- Scene 전이

주요 상태:

- `_CurrentSceneAsset`
- `_CurrentSceneInstance`
- `_CurrentShotNode`
- `_CurrentShotInstance`
- `_CurrentBranchNode`
- `_PendingStartRef`
- `_PendingTargetLevel`
- `_PendingTravelPhase`

---

# 4. Runtime 시작 흐름

## 4.1 시작 API

- `StartFromScene(SceneID)`
- `StartFromRef(FStoryFlowRef)`

의미:

- `SceneID`만 주면 EntryShot부터 시작
- `SceneID + ShotID`를 주면 특정 Shot부터 시작

## 4.2 시작 시 레벨 판정

1. `SceneRegistry`에서 SceneAsset 찾기
2. SceneAsset의 `TargetLevel` 확인
3. 현재 레벨과 다르면 로딩 절차 진입
4. 같으면 바로 Scene 시작

---

# 5. Runtime 진행 흐름

## 5.1 Shot 시작

1. `MoveToShot(ShotID)`
2. `SceneAsset.FindShotNode()`
3. ShotTemplate 찾기
4. `DuplicateObject<UStoryShotBase>(template, this)`
5. `InitializeShot(ShotID)`
6. `EnterShot()`

---

## 5.2 Shot 종료 후 다음 링크 결정

1. Tick에서 `IsFinished()` 감지
2. `ExitShot()` 먼저 호출
3. 현재 Shot의 `NextLink` 확인
4. 링크 타입에 따라 분기
   - `NextShotID`면 다음 Shot 이동
   - `NextBranchID`면 Branch 평가
   - `NextSceneID`면 다음 Scene 시작
5. 링크가 없으면 Scene 종료

중요:

- 현재 그래프 에디터 기준으로 Shot 노드는 다음 링크 1개를 가진다
- 실제 다중 갈래 선택은 Branch 노드가 담당한다

---

## 5.3 Branch 평가

1. `EvaluateBranch(BranchID, CurrentRef)`
2. `SceneAsset.FindBranchNode()`
3. BranchTemplate 찾기
4. `DuplicateObject<UStoryBranchBase>(template, this)`
5. `InitializeBranch(CurrentRef)`
6. `BranchCount`가 2개 이상이면 `SelectNextIndex(BranchCount)` 호출
   - `BranchCount`가 1이면 0번 링크를 그대로 사용
7. 선택된 Branch 링크로 이동
   - Shot이면 `MoveToShot()`
   - Scene이면 `StartFromScene()`

중요:

- Branch는 현재 Shot을 끝낸 다음 실행되는 **분기 판단 단계**다
- `EvaluateBranch()`에 들어가는 `FStoryFlowRef`는 현재 구현상 `SceneID`는 유지되지만, `ShotID`는 비어 있을 수 있다
  - 이유: Shot 종료 직후 `_CurrentShotNode`가 정리된 다음 Branch 평가가 호출되기 때문

---

## 5.4 Transition 처리

- Transition 노드는 런타임 인스턴스를 만들지 않는다
- Compile 결과로 `NextSceneID`만 남는다
- Shot 또는 Branch의 다음 링크가 `NextSceneID`를 가지면 `StartFromScene()`으로 넘긴다

---

# 6. Editor 구조

## 6.1 `FStoryFlowEditorModule`
파일:
- `Source/StoryFlowEditor/Public/StoryFlowEditorModule.h`
- `Source/StoryFlowEditor/Private/StoryFlowEditor.cpp`

역할:

- StoryFlow 에셋 카테고리 등록
- `StorySceneAsset` 에셋 타입 등록
- Scene/Shot/Branch Base Blueprint 팩토리 등록
- StoryFlow 클래스 아이콘 스타일 등록
- ID property customization 등록
- Shot 노드 커스텀 UI 등록
- PIE authorizer 등록

추가된 에디터 편의 기능:

- `Add New -> StoryFlow` 아래에 Base Blueprint 생성 바로가기 제공
- Scene/Shot/Branch Base 아이콘을 각각 다르게 표시
- `SceneID` / `ShotID` / `BranchID`를 한 줄 UI로 편집 가능

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

특징:

- 새 에셋 생성 시 기본 `SceneID = AssetName`

---

## 6.4 `UStorySceneBaseBlueprintFactory` / `UStoryShotBaseBlueprintFactory` / `UStoryBranchBaseBlueprintFactory`
파일:
- `Source/StoryFlowEditor/Public/StoryFlowBlueprintFactories.h`
- `Source/StoryFlowEditor/Private/StoryFlowBlueprintFactories.cpp`

기능:

- 우클릭 `Add New -> StoryFlow` 아래에서 바로 생성 가능
  - `StoryFlow Scene Blueprint`
  - `StoryFlow Shot Blueprint`
  - `StoryFlow Branch Blueprint`
- 각 팩토리는 부모 클래스를 고정한다
  - `UStorySceneBase`
  - `UStoryShotBase`
  - `UStoryBranchBase`
- 각 클래스는 서로 다른 엔진 기본 아이콘을 사용해 구분된다
  - Scene: `World`
  - Shot: `CineCameraActor`
  - Branch: `BehaviorTree`

---

## 6.5 `FStorySceneEditor`
파일:
- `Source/StoryFlowEditor/Private/StorySceneEditor.h`
- `Source/StoryFlowEditor/Private/StorySceneEditor.cpp`

Story Scene 전용 에디터.

구성:

- Graph 탭
- Details 탭
- Compile 버튼

기능:

- 그래프 생성 / 열기
- Details 편집
- Compile
- Copy / Cut / Paste / Delete / Undo / Redo
- PIE 전 compile 검증
- Shot / Branch / Transition 선택 시 대응 객체를 Details에 표시
- Details 변경 시 Branch 핀 수를 포함한 그래프 노드 핀 동기화

---

## 6.6 `UStorySceneEdGraph`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneEdGraph.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneEdGraph.cpp`

역할:

- Entry 노드 보장
- Entry에서 도달 가능한 노드 집합 계산
- 그래프를 런타임 데이터로 재구성 (`RebuildRuntimeData`)

중요:

- 런타임 데이터 동기화 책임은 GraphSchema가 아니라 `RebuildRuntimeData()`
- Compile 시 다음 런타임 데이터로 접어 넣는다
  - `EntryShotID`
  - Shot `_NextLink`
  - Branch `_NextLinksByPinIndex`
- 현재 Compile은 **`Entry`에서 도달 가능한 노드만** 대상으로 한다
- 미연결 노드는:
  - 런타임 데이터 재구성 대상에서 제외
  - 유효성 검사 대상에서도 제외
  - 실행 경로에 포함되지 않는다

---

## 6.7 `UStorySceneGraphNode_Entry`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Entry.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Entry.cpp`

특징:

- 삭제 불가
- 복제 불가
- `Next` 출력 핀 1개

---

## 6.8 `UStorySceneGraphNode_Shot`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Shot.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Shot.cpp`

역할:

- 생성 시 `UStorySceneNodeData` 생성
- 삭제 시 대응 NodeData 제거
- 복사/붙여넣기 시 NodeData 소유권 복구 및 재등록
- 노드 제목 표시
- 입력 `In`, 출력 `Next` 핀 유지

중요:

- 생성 시 ShotID를 `Shot_###` 형식으로 자동 부여한다
- 현재 코드 기준으로 Shot 노드는 다중 출력 핀을 직접 갖지 않는다
- `Ctrl+C / Ctrl+V` 복제 지원
  - Details 설정값은 유지
  - `ShotID`는 새로 재발급

현재 추가된 편의 기능:

- 노드 바깥 우측 상단 Play 버튼
- 버튼 클릭 시 해당 Shot부터 PIE 시작
- PIE 중 버튼 숨김

Desc 표시:

- 툴팁이 아니라 **comment bubble**
- Compile 시 `NodeComment`로 동기화

---

## 6.9 `UStorySceneGraphNode_Branch`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Branch.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Branch.cpp`

역할:

- 생성 시 `UStoryBranchNodeData` 생성
- 삭제 시 대응 BranchNodeData 제거
- 복사/붙여넣기 시 NodeData 소유권 복구 및 재등록
- 입력 `In`과 다중 출력 `Next_0`, `Next_1` ... 핀 유지
- Details의 `BranchCount` 변화에 맞춰 출력 핀 수 동기화

중요:

- 생성 시 BranchID를 `Branch_###` 형식으로 자동 부여한다
- 새 노드 오토와이어 시 비어 있는 `Next_*` 핀을 우선 사용한다
- Compile 에러 메시지를 노드 단위로 표시한다
- `Ctrl+C / Ctrl+V` 복제 지원
  - Details 설정값은 유지
  - `BranchID`는 새로 재발급

---

## 6.10 `UStorySceneGraphNode_Transition`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Transition.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphNode_Transition.cpp`

역할:

- 다음 Scene 전이용 노드
- `NextSceneID`를 가짐
- 출력 핀은 없고 입력 핀만 가진다

특징:

- `Ctrl+C / Ctrl+V` 복제 지원
- 복제 시 `NextSceneID` 유지
- 연결되어 있는데 `NextSceneID`가 비어 있으면 Compile 에러 대상

---

## 6.11 `UStorySceneGraphSchema`
파일:
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphSchema.h`
- `Source/StoryFlowEditor/Private/Graph/StorySceneGraphSchema.cpp`

그래프 연결 규칙 담당.

현재 규칙:

- 컨텍스트 메뉴에서 `Add Shot`, `Add Branch`, `Add Transition` 제공
- `Entry -> Transition` 금지
- `Transition -> *` 금지
- `Branch -> Branch` 금지
- 모든 출력 핀은 1개 연결만 허용
- 모든 입력 핀은 1개 연결만 허용
- 연결 생성/해제 시 그래프 변경 알림을 즉시 발생시켜 에디터 상태를 동기화

허용 연결:

- `Entry -> Shot`
- `Shot -> Shot / Branch / Transition`
- `Branch -> Shot / Transition`

---

# 7. Compile 동작

`StorySceneEditor`의 Compile 버튼은 다음 순서로 동작한다.

1. ShotID 자동 보정
2. Desc를 comment bubble용 `NodeComment`로 동기화
3. `Entry`에서 **도달 가능한 노드 집합** 계산
4. 그래프 기준 런타임 데이터 재생성
   - `EntryShotID`
   - Shot `_NextLink`
   - Branch `_NextLinksByPinIndex`
5. 무결성 검사

중요:

- 현재 Compile은 **`Entry`에서 도달 가능한 노드만** 대상으로 한다
- 즉, 그래프에 존재하더라도 미연결 노드는:
  - 런타임 데이터 재구성 대상에서 제외
  - 유효성 검사 대상에서도 제외
  - 실행 경로에 포함되지 않는다

---

# 8. Compile 무결성 검사

현재 검사 항목:

## Scene 수준

- `SceneID`가 비어 있지 않은가
- `StoryFlowDeveloperSettings.LoadingLevel`이 설정되어 있는가
- `TargetLevel`이 설정되어 있는가
- `LoadingLevel`과 `TargetLevel`이 같은 레벨이 아닌가
- `SceneTemplate`이 설정되어 있는가
- `SceneID`가 `StorySceneRegistry`에 등록되어 있는가
- `SceneID`가 `StorySceneRegistry`에 중복 등록되어 있지 않은가
- `EntryShotID`가 유효한가
- `EntryShotID`가 실제 ShotNodes 안에 존재하는가

## Shot 수준

- `ShotID`가 비어 있지 않은가
- `ShotID`가 중복되지 않는가
- `ShotTemplate`이 설정되어 있는가
- `StorySceneRegistry`가 있는가
- `NextLink`가 실존 Shot / Branch / Registry 등록 Scene을 가리키는가
- `NextLink` 안에 유효하지 않은 링크가 없는가

## Branch 수준

- `BranchID`가 비어 있지 않은가
- `BranchID`가 중복되지 않는가
- `BranchTemplate`이 설정되어 있는가
- 모든 출력 pin index가 유효 범위 안에 있는가
- 모든 필수 출력 pin이 연결되어 있는가
- `NextLinksByPinIndex`가 실존 Shot 또는 Registry 등록 Scene을 가리키는가
- `NextLinksByPinIndex` 안에 유효하지 않은 링크가 없는가

## Transition 수준

- 연결된 Transition이면 `NextSceneID`가 있어야 하는가
- `NextSceneID`는 Registry에 등록된 Scene인가
- `NextSceneID`가 Registry에 중복 등록된 Scene은 아닌가

오류가 있으면:

- Compile 실패
- 노드 하단 빨간 오류 메시지
- PIE 시작 차단

---

# 9. 로딩 관련 현재 상태

현재 구현됨:

- `LoadingLevel` 진입
- `LoadingLevel` 필수 강제
- `TargetLevel` async load
- UI에서 읽을 수 있는 `GetTargetLevelLoadingProgressRate()` 제공
- `MinimumLoadingLevelDuration` 충족 전까지 TargetLevel 오픈 지연
- `MinimumLoadingLevelProgressCurve`를 통한 표시용 progress 곡선 보정

현재 미구현:

- 로딩 UI 위젯 구현 자체
- 최소 체류시간 완료 전용 연출
- 세밀한 progress bar

---

# 10. 저장 / 복원

저장 최소 단위:

- `SceneID`
- `ShotID`

저장 기준:

- 저장은 **현재 진행 중인 Shot**을 기준으로 한다
- 현재 진행 중인 `ShotID`를 저장한다
- Branch는 지속 상태가 아니라 순간적인 판단 단계이므로 저장 상태로 취급하지 않는다
- 따라서 Branch 평가 중간 상태를 저장/복원하지 않는다

복원:

- `StartFromRef(FStoryFlowRef)`
- 같은 레벨이면 바로 시작
- 다른 레벨이면 레벨 전환 후 시작

---

# 11. 현재 장점

- Scene / Shot / Branch 역할이 분리됨
- SceneTemplate / ShotTemplate / BranchTemplate 패턴이 일관됨
- 레벨 준비 후 Scene 시작 구조 확보
- Branch 노드로 다중 분기 표현 가능
- Transition을 통해 다음 Scene 전이 가능
- 저장/복원이 `SceneID + ShotID`로 단순함
- 에디터 편집과 런타임 실행이 분리됨
- 미연결 노드를 compile/runtime 경로에서 자동 제외할 수 있음
- `Shot` / `Branch` / `Transition` 노드의 복제 편의성이 있음
- ID 계열(`SceneID`, `ShotID`, `BranchID`)을 한 줄 UI로 편집 가능
- Scene/Shot/Branch Base Blueprint를 StoryFlow 카테고리에서 바로 생성 가능

---

# 12. 현재 한계

- 저장/복원은 의도적으로 `SceneID + ShotID` 기준이다
- Branch는 저장 가능한 상태가 아니라는 전제를 유지한다
- 로딩 UI 자체는 아직 구현되어 있지 않다

---

# 13. 추천 코드 읽기 순서

1. `StoryFlowDefines.h`
2. `StorySceneBase.h`
3. `StoryShotBase.h`
4. `StoryBranchBase.h`
5. `StorySceneNodeData.h`
6. `StoryBranchNodeData.h`
7. `StorySceneAsset.h`
8. `StorySceneRegistryAsset.h`
9. `StoryFlowDeveloperSettings.h`
10. `StoryFlowSubsystem.h/.cpp`
11. `StoryFlowEditor.cpp`
12. `StorySceneEditor.cpp`
13. `StorySceneEdGraph.cpp`
14. `StorySceneGraphSchema.cpp`

---

# 14. 한 문장 요약

> **StoryFlow는 Scene(장소) 단위 에셋에 TargetLevel / SceneTemplate / Shot 흐름을 정의하고, Shot은 단일 다음 링크만 가지며 Branch 노드가 다중 갈래를 결정하고, Transition을 통해 다음 Scene으로 전이하며, 필요 시 LoadingLevel을 거쳐 TargetLevel을 비동기 준비한 뒤 SceneTemplate와 Shot을 순차 실행하는 스토리 흐름 플러그인**이다.
