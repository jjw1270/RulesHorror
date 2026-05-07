# RulesHorror / 마지막 특종

> 특종을 좇는 몰락한 기자가 정체불명의 사이트와 세 개의 낙인을 따라가며 자신이 묻어둔 과거의 심판에 도달하는 Unreal Engine 5 기반 스토리형 호러 게임 프로토타입입니다.
> 프로젝트명은 `RulesHorror`, 기획상 정식 게임 제목은 **마지막 특종**입니다.

**주의**

본 프로젝트의 자체 제작 코드, 기획 문서, 프로젝트 구성 및 게임 IP의 저작권은 **장윤제**에게 있습니다.

외부 에셋의 라이선스와 저장소 용량 문제로 일부 상용/외부 에셋과 로컬 빌드 산출물은 저장소에 포함하지 않았습니다.

이 저장소는 **Steam 출시를 목표로 개발 중인 게임 프로젝트**를 포트폴리오 검토가 가능하도록 공개한 형태입니다. 따라서 현재 저장소만으로 완성 빌드 실행을 보장하기보다는, 게임 클라이언트 구조와 시스템 구현을 확인하는 데 초점을 둡니다.

---

## 프로젝트 개요

`마지막 특종`은 좌천된 신문사 기자가 수신불명의 제보 전화를 계기로 괴이한 사이트에 접속하고, 세 개의 낙인을 모으는 과정에서 자신이 묻어두었던 추악한 과거와 죄의 대가를 마주하게 되는 스토리형 호러 게임입니다.

프로젝트의 최종 목표는 프로토타입 검증을 넘어, 핵심 플레이 루프와 연출 파이프라인을 완성해 **Steam 출시 가능한 단편 호러 게임**으로 발전시키는 것입니다.

기획상 플레이 구조는 다음 흐름을 목표로 합니다.

```text
신문사 도입
→ 제보 전화 / 사고
→ 꿈속 허브
→ 정체불명의 사이트
→ 세 개의 낙인 시나리오
→ 추격 세트피스
→ 병실 엔딩
```

현재 저장소에서 특히 강조할 수 있는 구현 포인트는 다음과 같습니다.

- UE5 C++ 기반 게임 클라이언트 구조
- 기능별 플러그인 분리
- 1인칭 탐색과 상호작용 시스템
- 월드 안 컴퓨터를 조작하는 디제틱 UI
- 모니터 위젯 입력 보정
- 브라우저형 멀티 윈도우 UI
- DataTable / Registry 기반 스토리 콘텐츠 연결
- 그래프 기반 `Scene / Shot / Branch` 스토리 플로우 플러그인

---

## 개발 환경

| 항목 | 내용 |
| --- | --- |
| Engine | Unreal Engine `5.7` |
| Target Platform | Windows |
| Main Language | C++ |
| UI | UMG / Slate |
| Input | Enhanced Input |
| 주요 UE 모듈 | `UMG`, `Slate`, `EnhancedInput`, `DeveloperSettings` |
| 주요 자체 모듈 | `InteractionSystem`, `ItemCore`, `StoryFlow`, `CustomUI`, `SaveGame`, `CommonLibrary` |

프로젝트 설정은 `RulesHorror.uproject`와 `Source/RulesHorror/RulesHorror.Build.cs`를 기준으로 확인할 수 있습니다.

---

## 저장소 포함/제외 기준

README의 구조와 규모 설명은 **Git 추적 파일 기준**입니다. 다음 항목은 `.gitignore`에 의해 제외됩니다.

- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`
- `Plugins/**/Binaries/`, `Plugins/**/Intermediate/`
- `Content/Assets/`의 실제 외부 에셋 팩
- `기획/레퍼런스/`
- `포트폴리오/`, `ToFix/`, `.omx/`
- `UE_EngineDir.txt`, `*.sln`, `*.dll`, `*.lib`, `*.exe`, `*.pdb` 등 로컬 생성물

현재 Git 추적 기준 대략적인 규모는 다음과 같습니다.

| 구분 | 규모 |
| --- | ---: |
| Git tracked entries | 355 |
| 일반 파일 | 352 |
| `Source/RulesHorror` | 95 files / 약 3.9k lines |
| `Plugins` 추적 항목 | 151 entries |
| `Plugins` 내 추적 C++/Build 코드 | 113 files / 약 9.0k lines |
| `Content` | 89 files |
| tracked map | 4 `.umap` |
| tracked asset | 84 `.uasset` |
| 기획 문서 | 3 files |

---

## 전체 구조

```text
RulesHorror/
├─ Config/                         # UE 프로젝트 설정
├─ Content/                        # 맵, BP, UI, DataTable, Story 에셋
│  ├─ GameFramework/               # GameMode / Pawn / PlayerController BP
│  ├─ Input/                       # Enhanced Input Action / Mapping Context
│  ├─ Maps/                        # L_Loading, L_Lobby, L_Office 등
│  ├─ Objects/                     # Interactable Actor BP
│  ├─ Story/                       # StoryFlow 에셋, Shot, Branch
│  ├─ Tables/                      # Item DataTable, StringTable
│  └─ UI/                          # Common / Lobby / Office / Monitor UI
├─ Source/
│  └─ RulesHorror/                 # 게임 본체 C++ 모듈
│     ├─ GameFramework/            # GameMode, Pawn, PlayerController, CameraManager
│     ├─ Item/                     # 프로젝트 전용 Story Item Row / Helper
│     ├─ Objects/Interactable/     # Computer 등 월드 상호작용 Actor
│     ├─ SaveGame/                 # 프로젝트 SaveGame 타입
│     ├─ StoryFlow/Branches/       # 프로젝트 전용 StoryFlow Branch
│     └─ UI/                       # 로딩, 로비, 오피스, 모니터 UI C++
├─ Plugins/
│  ├─ CommonLibraryPlugin/         # 공통 유틸 플러그인, submodule
│  ├─ CustomUIPlugin/              # 공통 UI 프레임워크, submodule
│  ├─ SaveGamePlugin/              # 저장 시스템, submodule
│  ├─ InteractionSystem/           # 상호작용 감지/표시/실행 플러그인
│  ├─ ItemCorePlugin/              # ItemID / Registry / DataTable 플러그인
│  ├─ RulesPlugin/                 # 규칙 시스템 확장용 빈 플러그인/실험 자리
│  └─ StoryFlow/                   # Scene / Shot / Branch 스토리 플로우 플러그인
├─ 기획/                           # 개발/스토리 기획 문서
├─ RulesHorror.uproject
├─ GenerateProjectFiles.bat
├─ BuildAndLaunch.bat
└─ SyncBranch.bat
```

---

## 기획 문서와 구현 연결

| 문서 | 역할 | 구현 연결 |
| --- | --- | --- |
| [`기획/개발기획.md`](./기획/개발기획.md) | MVP 구조, 코어 시스템, 구현 우선순위 | StoryFlow, SaveGame, 사이트/챕터 구조, 세로 슬라이스 |
| [`기획/모니터UI인터랙션_기술정리.md`](./기획/모니터UI인터랙션_기술정리.md) | 월드 상호작용형 컴퓨터, 모니터 입력 보정, 멀티 윈도우 UI를 기술 구현 관점에서 정리한 문서 | `AComputer`, `AOfficePawn`, `UUI_Monitor`, `UI_WindowManager`, `Site_StoryList/Detail` |
| [`기획/스토리기획.md`](./기획/스토리기획.md) | 시놉시스, 인물, 챕터 흐름, 실제 에셋명 기준 StoryFlow Scene / Shot 분해 | `Content/Story`, `StoryFlow`, `RulesHorrorDeveloperSettings` |

이 프로젝트는 기획 문서의 흐름을 그대로 코드에 하드코딩하기보다, 다음과 같이 콘텐츠 제작 단위로 분해하는 방향을 잡았습니다.

```text
Chapter / Scene 기획
→ StoryFlow Scene Asset
→ Shot / Branch 단위 실행 로직
→ DataTable 기반 콘텐츠 참조
→ BP / UI / Map 배치
```

---

## 핵심 시스템

### 1. 플러그인 중심 모듈 구조

게임 본체인 `RulesHorror` 모듈은 여러 기능 플러그인을 조합하는 형태로 구성됩니다.

| 플러그인 | 역할 | 저장소 연결 방식 / 참조 |
| --- | --- | --- |
| `CommonLibraryPlugin` | 로그, 유효성 검사, 공통 유틸리티 | Git submodule / [CommonLibraryPlugin](https://github.com/jjw1270/CommonLibraryPlugin.git) |
| `CustomUIPlugin` | `WidgetBase`, Popup, Button, StringTable, Widget Registry 등 공통 UI 기반 | Git submodule / [CustomUIPlugin](https://github.com/jjw1270/CustomUIPlugin.git) |
| `InteractionSystem` | 상호작용 대상 감지, Target 표시, Overlay, Indicator UI | Repo-local tracked source / [`Plugins/InteractionSystem`](./Plugins/InteractionSystem) |
| `ItemCorePlugin` | `ItemID`, DataTable Row, Registry, 에디터 Picker | Git subtree / [ItemCorePlugin](https://github.com/jjw1270/ItemCorePlugin.git) |
| `SaveGamePlugin` | SaveGame Subsystem과 저장 슬롯 관리 | Git submodule / [SaveGamePlugin](https://github.com/jjw1270/SaveGamePlugin.git) |
| `StoryFlow` | Scene / Shot / Branch 그래프 기반 스토리 진행 | Repo-local tracked source / [`Plugins/StoryFlow`](./Plugins/StoryFlow) |
| `RulesPlugin` | 규칙형 콘텐츠 확장을 위해 자리만 잡아둔 플러그인. 현재 실질 기능은 거의 없음 | Repo-local tracked source / [`Plugins/RulesPlugin`](./Plugins/RulesPlugin) |

Submodule 플러그인은 GitHub에서 일반 폴더처럼 바로 펼쳐지지 않고 gitlink로 보일 수 있으므로, 위 표의 GitHub 링크를 함께 참고하는 것이 좋습니다.
Subtree로 가져온 플러그인은 현재 저장소에 코드가 포함되어 있지만, 원본 추적 정보는 `SubtreeList.txt`에 남겨두었습니다.

관련 파일:

- `RulesHorror.uproject`
- `Source/RulesHorror/RulesHorror.Build.cs`
- `Plugins/*/*.uplugin`
- `.gitmodules`
- `SubtreeList.txt`

---

### 2. 1인칭 탐색과 상호작용

`InteractionSystem` 플러그인은 플레이어가 주변 Actor를 감지하고, 화면 중앙 또는 커서 기준으로 Target을 선택한 뒤 `Interact`를 호출하는 구조입니다.

주요 특징:

- `UInteractorComponent` 기반 감지
- `IInteractableInterface` 기반 상호작용 Actor 확장
- `CameraCenter` / `Cursor` 감지 모드 분리
- `None`, `Detected`, `Targeted` 상태 관리
- Targeted Actor Overlay Material 적용
- Editor Component Visualizer 지원

관련 파일:

- `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractorComponent.h`
- `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractableInterface.h`
- `Plugins/InteractionSystem/Source/InteractionSystem/Public/InteractionSystemDefines.h`
- `Source/RulesHorror/GameFramework/Pawn/InteractionPawn/InteractionPawn.*`

---

### 3. 디제틱 컴퓨터 / 모니터 UX

로비와 오피스의 컴퓨터는 단순 메뉴 버튼이 아니라, 월드에 배치된 Actor를 상호작용해 모니터 UI로 진입하는 방식입니다.

`AComputer`는 다음 역할을 담당합니다.

- 모니터 Mesh와 WidgetComponent 관리
- 전원 On/Off 상태 제어
- Interactable Interface 구현
- 상호작용 시 OfficePawn을 지정 MovePoint로 이동
- 상호작용 전용 UI 열기/닫기

이 디제틱 컴퓨터 UX의 입력 보정, 화면 상태 전환, 멀티 윈도우 구조, DataTable 기반 사이트 연결은 별도 기술 문서 [`기획/모니터UI인터랙션_기술정리.md`](./기획/모니터UI인터랙션_기술정리.md)에 더 자세히 정리되어 있습니다.

관련 파일:

- `Source/RulesHorror/Objects/Interactable/Computer.h`
- `Source/RulesHorror/Objects/Interactable/Computer.cpp`
- `Source/RulesHorror/GameFramework/Pawn/OfficePawn.*`
- `Content/Objects/Interactable/Computer_BP.uasset`
- `Content/UI/Office/UI_OnInteractingComputer_BP.uasset`

---

### 4. 모니터 위젯 입력 보정

`AOfficePawn`은 `UWidgetInteractionComponent`를 Custom Hit Result 방식으로 사용합니다. 마우스 커서가 월드의 모니터 표면을 가리킬 때, Hit 결과를 보정해 모니터 내부 UI 좌표로 전달합니다.

주요 흐름:

```text
PlayerController cursor hit
→ Monitor trace channel 판정
→ 보정된 Widget Hit 생성
→ WidgetInteractionComponent::SetCustomHitResult
→ 모니터 내부 커서 / 버튼 / 스크롤 처리
```

관련 파일:

- `Source/RulesHorror/GameFramework/Pawn/OfficePawn.h`
- `Source/RulesHorror/GameFramework/Pawn/OfficePawn.cpp`
- `Source/RulesHorror/UI/Office/UI_Monitor.*`
- `Source/RulesHorror/UI/Office/UI_Cursor.*`

포트폴리오 관점에서는 **3D 공간의 오브젝트 표면과 2D UMG 입력을 연결한 사례**로 볼 수 있습니다.

---

### 5. 브라우저형 멀티 윈도우 UI

모니터 UI는 단일 화면 전환 위젯이 아니라, 데스크톱처럼 창을 열고 닫는 구조를 갖습니다.

구현된 개념:

- Window Manager
- Window Base
- Window Layout
- Window Tab
- Window Icon
- Minimize / Restore / Close
- Move / Resize
- Explorer Window
- Story List / Story Detail Site

관련 파일:

- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/UI_WindowManager.*`
- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/WindowBase.*`
- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/WindowDefines.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/Window_Explorer.*`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/StoryListSite/Site_StoryList.*`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/StoryDetailSite/Site_StoryDetail.*`

---

### 6. DataTable 기반 스토리 콘텐츠 연결

스토리 사이트에서 표시할 항목은 코드에 직접 박아 넣기보다 `ItemCore`의 DataTable / Registry 구조 위에 얹어 관리합니다.

프로젝트 전용 Row인 `FStoryTableRow`는 `FItemTableRow`를 상속하며 다음 정보를 추가합니다.

- `AdditionalSiteAddress`
- `StoryMap`

관련 파일:

- `Source/RulesHorror/Item/RulesHorrorItemDefines.h`
- `Source/RulesHorror/Item/RulesHorrorItemHelper.h`
- `Source/RulesHorror/Item/RulesHorrorItemHelper.cpp`
- `Content/Tables/Items/DA_ItemTables.uasset`
- `Content/Tables/Items/DT_Item_Story.uasset`
- `Content/Tables/StringTables/*.uasset`

이 구조를 통해 스토리 제목, 사이트 주소, 연결 맵 같은 콘텐츠 데이터를 코드 수정 없이 확장하는 방향을 지향합니다.

---

### 7. StoryFlow 기반 스토리 진행

`StoryFlow`는 장소/상황 단위 `Scene`과 그 안의 진행 단계인 `Shot`, 조건 분기인 `Branch`를 그래프로 구성하는 자체 플러그인입니다.
현재는 단순 후보 시스템이 아니라, **로비 인트로 / 새 게임 / 이어하기 진입 흐름에 실제 연결된 상태**입니다.

핵심 개념:

```text
StorySceneAsset
├─ Entry
├─ Shot
├─ Branch
└─ Transition
```

현재 실제 ID / 자산 규칙:

```text
SceneID        : CH##_S##
StorySceneAsset: CH##_S##_Asset
Shot Asset     : S##_###
Template Shot  : Shot_*
Template Branch: Branch_*
```

현재 확인되는 실제 자산:

- `Content/Story/DA_StorySceneList.uasset`
- `Content/Story/Intro/SA_Intro.uasset`
- `Content/Story/Intro/Scene_Intro.uasset`
- `Content/Story/Intro/Branch_ShouldLoadGame.uasset`
- `Content/Story/Intro/Shot_LoadGame.uasset`
- `Content/Story/Chapter1/Scene1/CH01_S01_Asset.uasset`
- `Content/Story/Chapter1/Scene1/CH01_S01.uasset`
- `Content/Story/Chapter1/Scene1/S01_001.uasset`
- `Content/Story/Shot_ShowWidget.uasset`
- `Content/Story/Shot_EnableInteraction.uasset`
- `Content/Story/Shot_SetInteractionDetectMode.uasset`
- `Content/Story/Curve_LoadingGauage.uasset`

주요 특징:

- `SceneID`, `ShotID`, `BranchID` 기반 식별
- `FStoryFlowRef(SceneID + ShotID)` 기준 저장/복원
- Shot 실행 객체를 런타임에서 복제해 실행
- Branch를 순간 판단 노드로 취급
- Transition을 다음 Scene 전이 링크로 Compile
- Story Scene 전용 에디터 / 그래프 / Compile 검증 지원
- 특정 Shot부터 PIE 실행하는 에디터 워크플로우 지원
- `RulesHorrorDeveloperSettings`의 `_IntroSceneID=Scene_Intro`, `_StoryStartSceneID=CH01_S01`와 연결되며, Intro용 StorySceneAsset 파일은 `SA_Intro.uasset`이다.

관련 파일:

- `Plugins/StoryFlow/Source/StoryFlow/Public/StoryFlowSubsystem.h`
- `Plugins/StoryFlow/Source/StoryFlow/Public/StorySceneAsset.h`
- `Plugins/StoryFlow/Source/StoryFlow/Public/StoryShotBase.h`
- `Plugins/StoryFlow/Source/StoryFlow/Public/StoryBranchBase.h`
- `Plugins/StoryFlow/Source/StoryFlowEditor/Private/StorySceneEditor.cpp`
- `Source/RulesHorror/RulesHorrorDeveloperSettings.h`
- `Source/RulesHorror/RulesHorrorGameInstance.cpp`
- `Source/RulesHorror/GameFramework/GameMode/LobbyGameMode.cpp`
- `Source/RulesHorror/SaveGame/RulesHorrorSaveGame.h`
- `Content/Story/DA_StorySceneList.uasset`
- `Content/Story/Intro/*`
- `Content/Story/Chapter1/Scene1/*`

---

## 현재 구현 상태

현재 저장소 기준으로 확인 가능한 구현입니다.

### 구현되어 있는 기반

- 로딩 / 로비 / 오피스 / 테스트 맵
- GameMode / Pawn / PlayerController BP 구조
- Enhanced Input Action / Mapping Context
- 상호작용 Pawn과 이동 포인트 기반 상호작용
- Computer Actor와 모니터 UI 진입 흐름
- Office 모니터용 커서 / WidgetInteraction 처리
- 멀티 윈도우 기반 모니터 UI
- Explorer / Story List / Story Detail UI 코드
- Story Item DataTable 연결 구조
- `SA_Intro` StorySceneAsset + `Scene_Intro` SceneID → 새 게임 / 이어하기 분기 → `CH01_S01` 시작 흐름
- `FStoryFlowRef` 기반 StoryFlow 저장 / 복원 기초 구조
- StoryFlow 플러그인과 일부 Intro / Chapter1 Story 에셋
- ItemCore, InteractionSystem, StoryFlow Editor 모듈

### 아직 구현이 필요한 핵심

- 낙인 / 시나리오 / 엔딩 조건 Save Key 반영
- StoryFlow 중요 Shot마다 저장 갱신 정책 추가
- `CH01_S02` 이후 본편 StorySceneAsset / ShotAsset 제작
- 괴담 사이트 진행도 반영 (`USite_StoryList` TODO 구간)
- 꿈속 허브 상태 변화
- 미니게임 4종 본 구현
- 병실 엔딩 UX 완성

---

## 실행 및 빌드

이 프로젝트에는 Windows 환경에서 Git 동기화, UE 프로젝트 파일 생성, 에디터 빌드/실행을 반복하기 쉽게 만든 배치 자동화 스크립트가 포함되어 있습니다.

| 파일 | 역할 | 포트폴리오 포인트 |
| --- | --- | --- |
| `SyncBranch.bat` | Git fetch/pull, Subtree 동기화, Submodule 초기화/업데이트 | 외부 플러그인과 프로젝트 본체를 한 번에 맞추는 협업/동기화 자동화 |
| `GenerateProjectFiles.bat` | UE 엔진 경로 검증 후 Visual Studio 프로젝트 파일 생성 | 비개발자도 엔진 경로만 입력하면 프로젝트 파일을 재생성할 수 있게 한 환경 설정 자동화 |
| `BuildAndLaunch.bat` | Git/Subtree/Submodule 동기화 → 프로젝트 파일 생성 → `RulesHorrorEditor Win64 Development` 빌드 → `.uproject` 실행 | 디자이너/기획자가 소스 코드를 직접 보지 않아도 최신 프로젝트를 빌드하고 에디터를 열 수 있게 한 원클릭 파이프라인 |

### 1. 저장소 동기화

Submodule과 Subtree를 함께 사용합니다.

```bat
SyncBranch.bat
```

또는 수동으로 진행할 경우:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

Subtree 대상은 `SubtreeList.txt`를 참고합니다.

### 2. Visual Studio 프로젝트 파일 생성

```bat
GenerateProjectFiles.bat
```

최초 실행 시 UE 엔진 루트 경로를 입력합니다. 입력한 경로는 로컬 파일 `UE_EngineDir.txt`에 저장되며, 이 파일은 Git에 포함하지 않습니다.

### 3. 빌드 및 에디터 실행

```bat
BuildAndLaunch.bat
```

이 스크립트는 다음 작업을 순서대로 수행합니다.

1. Git fetch / pull 확인
2. Subtree 동기화
3. Submodule 동기화
4. Visual Studio 프로젝트 파일 생성
5. `RulesHorrorEditor Win64 Development` 빌드
6. `.uproject` 실행

> 일부 외부 에셋은 저장소에 포함되지 않으므로, 에디터 실행 시 누락 에셋 경고가 발생할 수 있습니다.

---

## 코드 읽기 추천 순서

프로젝트 구조를 빠르게 파악하려면 아래 순서로 보는 것을 추천합니다.

1. `RulesHorror.uproject`
2. `Source/RulesHorror/RulesHorror.Build.cs`
3. `Source/RulesHorror/GameFramework/Pawn/InteractionPawn/InteractionPawn.*`
4. `Source/RulesHorror/GameFramework/Pawn/OfficePawn.*`
5. `Source/RulesHorror/Objects/Interactable/Computer.*`
6. `Source/RulesHorror/UI/Office/UI_Monitor.*`
7. `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/*`
8. `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/*`
9. `Source/RulesHorror/Item/*`
10. `Plugins/InteractionSystem/README.md`
11. `Plugins/ItemCorePlugin/README.md`
12. `Plugins/StoryFlow/ReadMe.md`

---

## 포트폴리오에서 강조할 점

이 프로젝트는 단순 기술 데모가 아니라 **Steam 출시를 목표로 한 상용화 지향 호러 게임**입니다. 포트폴리오에서는 현재 구현된 코드와 제작 파이프라인을 통해 다음 역량을 보여주는 데 초점을 둡니다.

- UE5 C++ 모듈 의존성 설계
- 게임 본체와 재사용 플러그인의 책임 분리
- 상호작용 감지 로직의 컴포넌트화
- 월드 오브젝트와 UMG를 연결하는 디제틱 UI 구현
- 커서 / Hit / WidgetInteraction 보정 처리
- 데스크톱형 멀티 윈도우 UI 구현
- DataTable / Registry 기반 콘텐츠 확장 구조
- 스토리 진행을 Scene / Shot / Branch 단위로 도구화한 에디터 플러그인 설계
- 디자이너가 BP와 DataAsset으로 콘텐츠를 붙일 수 있는 제작 파이프라인 구성
- Git/Subtree/Submodule 동기화와 UE 빌드/실행을 묶은 Windows 배치 자동화 도구 제작

---

## 관련 문서

- [개발 기획](./기획/개발기획.md)
- [스토리 기획](./기획/스토리기획.md)
- [InteractionSystem README](./Plugins/InteractionSystem/README.md)
- [StoryFlow README](./Plugins/StoryFlow/ReadMe.md)
- Submodule 원본:
  - [CommonLibraryPlugin](https://github.com/jjw1270/CommonLibraryPlugin.git)
  - [CustomUIPlugin](https://github.com/jjw1270/CustomUIPlugin.git)
  - [SaveGamePlugin](https://github.com/jjw1270/SaveGamePlugin.git)
- Subtree 원본:
  - [ItemCorePlugin](https://github.com/jjw1270/ItemCorePlugin.git)

---

## 라이선스 / 에셋 안내

- 본 프로젝트의 자체 제작 소스 코드, 기획 문서, 문서화 자료, 프로젝트 구성 및 게임 IP의 저작권은 **장윤제**에게 있습니다.
- `Copyright (c) 2026 장윤제. All rights reserved.`가 명시된 파일은 해당 고지를 따릅니다.
- 이 저장소의 공개는 포트폴리오 열람과 기술 검토를 위한 것이며, 별도 허가 없이 복제, 수정, 재배포, 상업적 사용을 허용한다는 의미가 아닙니다.
- 외부 에셋, 레퍼런스 이미지, 빌드 산출물은 저장소에 포함하지 않으며, 외부 에셋의 권리는 각 원저작자와 라이선스 정책을 따릅니다.
- 최종 배포 목표는 Steam 출시이며, 공개 저장소는 출시 빌드가 아니라 개발/검토용 소스 저장소입니다.
