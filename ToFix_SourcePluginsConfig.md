# ToFix - Source Plugins Config Audit

작성일: 2026-04-15  
범위: Source/Plugins/Config/부트스트랩 스크립트의 플러그인 구성 상태 점검  
우선순위 기준: 치명적 실행 불가 > 빌드/설정 불일치 > 유지보수성 저하

## 실행한 근거 수집

- `git submodule status`
- `Get-ChildItem -Recurse Plugins/CommonLibraryPlugin, Plugins/CustomUIPlugin, Plugins/SaveGamePlugin`
- `rg -n "CommonLibrary|CustomUI|SaveGame|ItemCore|InteractionSystem" --glob '!Content/**'`
- `rg -n "RulesPlugin"`
- 파일 직접 확인:
  - `RulesHorror.uproject`
  - `.gitmodules`
  - `Config/DefaultGame.ini`
  - `Source/RulesHorror/RulesHorror.Build.cs`
  - `Plugins/InteractionSystem/InteractionSystem.uplugin`
  - `Plugins/ItemCorePlugin/ItemCore.uplugin`
  - `Plugins/RulesPlugin/RulesPlugin.uplugin`
  - `README.md`
  - `GenerateProjectFiles.bat`
  - `SyncBranch.bat`
  - `BuildAndLauch.bat`

---

## 1) [치명적] 필수 플러그인 서브모듈 3개가 비어 있어 현재 소스 그래프가 즉시 깨짐

### 근거

- `.gitmodules:1-9`  
  `Plugins/CommonLibraryPlugin`, `Plugins/CustomUIPlugin`, `Plugins/SaveGamePlugin` 이 모두 서브모듈로 선언되어 있음.
- `git submodule status` 결과
  - `-85f999fe... Plugins/CommonLibraryPlugin`
  - `-d36f60c1... Plugins/CustomUIPlugin`
  - `-dfbf697b... Plugins/SaveGamePlugin`
  
  앞의 `-` 는 현재 워크트리에 서브모듈 내용이 체크아웃되지 않았음을 뜻함.
- 실제 디렉터리 확인 결과
  - `Plugins/CommonLibraryPlugin => MISSING .uplugin`
  - `Plugins/CustomUIPlugin => MISSING .uplugin`
  - `Plugins/SaveGamePlugin => MISSING .uplugin`
- 그런데 코드/플러그인 설정은 해당 모듈들을 필수로 참조함.
  - `Source/RulesHorror/RulesHorror.Build.cs:23-28`
    - `CommonLibrary`
    - `CustomUI`
    - `ItemCore`
    - `InteractionSystem`
    - `SaveGame`
  - `Plugins/InteractionSystem/InteractionSystem.uplugin:31-35`
    - `CommonLibrary`, `CustomUI`
  - `Plugins/ItemCorePlugin/ItemCore.uplugin:31`
    - `CommonLibrary`
  - `Config/DefaultGame.ini:16-26`
    - `[/Script/CustomUI.CustomUIDeveloperSettings]`
    - `[/Script/ItemCore.ItemDeveloperSettings]`
    - `[/Script/SaveGame.SaveGameDeveloperSettings]`

### 영향

- 현재 상태 그대로는 프로젝트 파일 생성/빌드/에디터 로딩이 플러그인 해석 단계에서 실패할 가능성이 매우 높음.
- 특히 게임 모듈, 런타임 플러그인, 설정 파일이 모두 같은 외부 플러그인 집합에 결합되어 있어 부분 정상 동작 경로가 없음.

### 개선안

1. **가장 우선**: 초기 세팅 단계에서 `git submodule update --init --recursive` 를 강제한다.
2. `GenerateProjectFiles.bat` 시작 전에 `.uplugin` 존재 여부를 검사해서 누락 시 즉시 실패시키고 복구 명령을 안내한다.
3. CI/로컬 사전검사 스크립트에서 아래 조건을 검증한다.
   - `Plugins/CommonLibraryPlugin/**/*.uplugin` 존재
   - `Plugins/CustomUIPlugin/**/*.uplugin` 존재
   - `Plugins/SaveGamePlugin/**/*.uplugin` 존재

---

## 2) [높음] `.uproject` 플러그인 활성 목록과 실제 의존 그래프가 불일치

### 근거

- `RulesHorror.uproject:11-16` 에서 메인 모듈이 `CustomUI` 를 추가 의존성으로 선언함.
- `RulesHorror.uproject:19-38` 의 `Plugins` 목록에는 아래만 존재함.
  - `ModelingToolsEditorMode`
  - `StateTree`
  - `GameplayStateTree`
  - `ModuleGeneration`
- 반면 실제 코드/설정은 아래 프로젝트 플러그인들을 적극 사용함.
  - `CustomUI` (`RulesHorror.Build.cs:24`, `DefaultGame.ini:16`)
  - `ItemCore` (`RulesHorror.Build.cs:26`, `DefaultGame.ini:22`)
  - `InteractionSystem` (`RulesHorror.Build.cs:27`)
  - `SaveGame` (`RulesHorror.Build.cs:28`, `DefaultGame.ini:25`)

### 영향

- 팀원이 서브모듈만 받아도 `.uproject` 기준 프로젝트 구성이 명확하지 않다.
- UE가 로컬 프로젝트 플러그인을 자동 발견하더라도, **프로젝트에서 실제로 어떤 플러그인을 전제로 하는지 descriptor만 보고는 알기 어렵다.**
- 패키징/협업/온보딩 시 “왜 어떤 환경에서는 되고 어떤 환경에서는 안 되는지” 추적 비용이 커진다.

### 개선안

1. `RulesHorror.uproject` 에 실제 사용 중인 프로젝트 플러그인을 명시적으로 추가한다.
2. 최소한 문서에 “이 프로젝트는 `CommonLibrary`, `CustomUI`, `SaveGame`, `ItemCore`, `InteractionSystem` 을 전제로 한다”는 선언을 넣는다.
3. 플러그인 사용 여부가 선택 사항이 아니라면, `README` 와 부트스트랩 스크립트 모두 같은 목록을 단일 소스로 유지한다.

---

## 3) [중간] 부트스트랩 문서/스크립트 UX가 깨져 있어 초기 복구 경로가 불명확함

### 근거

- `README.md:6`  
  `SyncBranch.bat : subtree, submodule sync`
- `README.md:10`  
  `BuildAndLaunch.bat : 소스를 안보는 디자이너가 에디터를 실행할 경우 사용`
- 실제 파일명은 `BuildAndLauch.bat` 임. (`n` 누락)
- `BuildAndLauch.bat:161-198`, `SyncBranch.bat:143-179` 는 둘 다 서브모듈 동기화를 수행함.
- 반면 `GenerateProjectFiles.bat:108` 은 바로 project files 생성을 수행하며, 선행 플러그인 검증/서브모듈 동기화 단계가 없음.

### 영향

- README만 보고 따라가면 존재하지 않는 파일명을 만나게 된다.
- 가장 먼저 실행하기 쉬운 `GenerateProjectFiles.bat` 에는 현재 가장 중요한 실패 원인(서브모듈 누락) 방어가 없다.
- 결과적으로 실패 원인이 “엔진 경로 문제”처럼 보일 수 있어 디버깅 시간이 불필요하게 늘어난다.

### 개선안

1. `README` 의 파일명을 실제 파일명과 일치시킨다.  
   - 권장: 파일명을 `BuildAndLaunch.bat` 로 바로잡고 README도 같이 수정
2. `GenerateProjectFiles.bat` 초반에 필수 플러그인 `.uplugin` 존재 여부를 검사한다.
3. 문서의 권장 순서를 아래처럼 명시한다.
   1. `SyncBranch.bat`
   2. `GenerateProjectFiles.bat`
   3. `BuildAndLaunch.bat`

---

## 4) [중간] `RulesPlugin` 이 현재 프로젝트 기준 고립되어 있어 유지보수 비용만 남아 있음

### 근거

- `rg -n "RulesPlugin"` 결과가 `Plugins/RulesPlugin/**` 내부 자기참조만 반환함.
- `RulesHorror.uproject` 의 `Plugins` 목록에는 `RulesPlugin` 이 없음.
- 메인 게임 모듈 `RulesHorror.Build.cs` 도 `RulesPlugin` 을 의존하지 않음.

### 영향

- 현재 저장소 안에 “활성 플러그인인지, 보관용 실험 코드인지” 구분이 되지 않는 코드가 남아 있다.
- 팀원이 전체 구조를 읽을 때 실제 런타임 경로와 무관한 코드를 함께 추적하게 된다.

### 개선안

1. 실제 사용 계획이 없다면 제거 후보로 분류한다.
2. 유지할 계획이면 `README` 또는 플러그인 상단 문서에 용도와 활성 조건을 적는다.
3. 비활성 보관용이라면 별도 폴더/브랜치/문서로 의도를 분리한다.

---

## 정리

- **가장 먼저 고쳐야 할 문제는 “필수 서브모듈 플러그인 누락을 조기에 감지/복구하지 못하는 구조”** 다.
- 이 문제 하나로 빌드, 에디터 실행, 설정 로딩, 협업 온보딩이 모두 동시에 흔들린다.
- 그 다음 우선순위는 `.uproject` 의 플러그인 선언 명확화와 부트스트랩 문서/스크립트 일치화다.

## 검증 메모

- 문서 근거 재확인
  - `git diff --check` → 문서/정리 변경에 whitespace 오류 없음
  - `git submodule status` → 3개 필수 서브모듈이 미초기화 상태(`-` prefix)임을 재확인
  - `ConvertFrom-Json` on `RulesHorror.uproject`, `InteractionSystem.uplugin`, `ItemCore.uplugin`, `RulesPlugin.uplugin` → descriptor 파싱 성공
- 환경 한계
  - `UE_EngineDir.txt` 부재
  - `Plugins/CommonLibraryPlugin/*.uplugin`, `Plugins/CustomUIPlugin/*.uplugin`, `Plugins/SaveGamePlugin/*.uplugin` 모두 부재
  - 따라서 UE 빌드/에디터 기반 end-to-end 검증은 현재 워크트리 전제조건 미충족으로 수행 불가
