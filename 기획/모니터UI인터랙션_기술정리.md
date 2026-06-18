# 모니터 UI 인터랙션 기술 정리

> 대상 프로젝트: `RulesHorror / 마지막 특종`
> 
> 목적: 월드 상호작용형 컴퓨터 시스템을 **기술 구현 중심**으로 설명하기 위한 문서.

---

## 1. 기능 개요

이 기능은 단순히 메뉴를 띄우는 UI가 아니라, **월드에 배치된 컴퓨터 Actor를 플레이어가 직접 상호작용**하고, 그 결과로 **모니터 표면 기반 UI 조작**, **데스크톱형 멀티 윈도우 UX**, **스토리/시나리오 선택 흐름**까지 이어지도록 만든 디제틱 인터페이스 시스템이다.

핵심 목표는 다음과 같았다.

- 메뉴를 게임 외부 UI가 아니라 **게임 세계 내부 오브젝트**로 녹여내기
- 3D 월드의 모니터 표면과 2D UMG 입력을 자연스럽게 연결하기
- 단일 팝업 UI가 아니라 **브라우저/OS 느낌의 창 기반 UX**를 제공하기
- 스토리 콘텐츠를 코드 하드코딩 대신 **데이터 기반으로 확장 가능**하게 설계하기

---

## 2. 전체 구조

기능 흐름은 아래와 같다.

```text
플레이어가 컴퓨터를 감지
→ 상호작용 입력
→ 컴퓨터 Actor가 플레이어를 지정 위치로 이동
→ 모니터 전원 On + 전용 상호작용 UI 진입
→ 마우스 입력을 모니터 위젯으로 전달
→ 모니터 내부 멀티 윈도우 UI 조작
→ 사이트/스토리 선택
→ 선택된 맵 또는 시나리오로 진입
```

구성 레이어는 크게 5단계다.

1. **상호작용 감지 레이어**
2. **컴퓨터 Actor 및 진입 제어 레이어**
3. **모니터 표면 입력 처리 레이어**
4. **멀티 윈도우 UI 레이어**
5. **DataTable 기반 콘텐츠 연결 레이어**

---

## 3. 상호작용 감지 레이어

### 관련 파일
- `Plugins/InteractionPlugin/Source/InteractionSystem/Public/InteractableInterface.h`
- `Plugins/InteractionPlugin/Source/InteractionSystem/Public/InteractorComponent.h`
- `Plugins/InteractionPlugin/Source/InteractionSystem/Private/InteractorComponent.cpp`
- `Source/RulesHorror/GameFramework/Pawn/InteractionPawn/InteractionPawn.h`
- `Source/RulesHorror/GameFramework/Pawn/InteractionPawn/InteractionPawn.cpp`

### 구현 내용

상호작용 시스템은 프로젝트 공통 플러그인 형태로 분리되어 있다.

- `IInteractableInterface`
  - 상호작용 대상이 구현해야 할 인터페이스
  - 감지 가능 여부, 상호작용 가능 여부, 표시 이름, 상호작용 위치 등을 정의
- `UInteractorComponent`
  - 주변 Actor를 감지하고 현재 타겟을 선택하는 컴포넌트
  - `CameraCenter` / `Cursor` 두 가지 감지 모드를 지원
  - 감지 상태를 `None / Detected / Targeted`로 관리
  - Targeted Actor에 Overlay Material과 Indicator UI를 적용
- `AInteractionPawn`
  - 입력 수신과 이동 포인트 제어를 포함한 상호작용 Pawn 베이스 클래스

### 기술 포인트

- 상호작용 로직을 Actor 내부에 직접 흩뿌리지 않고, **인터페이스 + 컴포넌트 기반**으로 분리했다.
- 감지, 타겟 선택, 상태 반영, 시각 피드백을 `UInteractorComponent`가 일괄 관리한다.
- 추후 다른 상호작용 오브젝트를 추가할 때도 `IInteractableInterface`만 구현하면 동일한 시스템에 편입할 수 있다.

---

## 4. 컴퓨터 Actor 진입 제어

### 관련 파일
- `Source/RulesHorror/Objects/Interactable/Computer.h`
- `Source/RulesHorror/Objects/Interactable/Computer.cpp`
- `Source/RulesHorror/GameFramework/Pawn/OfficePawn.h`
- `Source/RulesHorror/GameFramework/Pawn/OfficePawn.cpp`
- `Source/RulesHorror/GameFramework/Pawn/InteractionPawn/InteractionPawnMovePoint.h`
- `Source/RulesHorror/GameFramework/Pawn/InteractionPawn/InteractionPawnMovePoint.cpp`

### 구현 내용

`AComputer`는 월드에 배치되는 실제 컴퓨터 오브젝트다.

주요 책임은 다음과 같다.

- 모니터 Mesh와 `WidgetComponent` 관리
- 렌더 타깃을 모니터 머티리얼에 연결
- 전원 On/Off에 따라 화면 표시 상태 제어
- `IInteractableInterface` 구현
- 상호작용 시 `AOfficePawn`을 컴퓨터 앞 `MovePoint`로 이동
- 상호작용 중 전용 UI 오픈/종료 처리

`AInteractionPawn`은 `MovePoint` 기반 이동 시스템을 가진다.

- 맵에 배치된 `AInteractionPawnMovePoint`를 이름으로 탐색
- 플레이어 상호작용 시 해당 포인트로 이동
- 이동 완료 후 Delegate로 후속 처리 연결
- 고정 카메라 여부와 상호작용 가능 여부도 포인트 단위로 제어

### 기술 포인트

- 상호작용 직후 곧바로 UI를 열지 않고, **플레이어 이동 → 도착 완료 → UI 오픈** 순서로 처리한다.
- 이를 통해 단순 메뉴 전환이 아니라, **“컴퓨터 앞에 접근해서 조작한다”**는 디제틱 연출을 만든다.
- `MovePoint`를 데이터처럼 써서, 코드 수정 없이 배치만 바꿔 연출 흐름을 조정할 수 있다.

### 핵심 코드

```cpp
void AComputer::Interact_Implementation(AActor* _interactor)
{
    _OfficePawn = Cast<AOfficePawn>(_interactor);
    if (IsInvalid(_OfficePawn))
        return;

    _OfficePawn->SetTargetMovePoint(_InteractMovePoint, false, _OnMoveToPointFinishedEvent);

    _OfficePawn->SetInteractingComputer(this);
    SetPower(true, true);
}
```

- 상호작용 성공 시 바로 UI를 띄우는 대신, 먼저 `OfficePawn`을 지정 위치로 이동시킨다.
- 이후 이동 완료 Delegate를 통해 후속 UI 진입을 연결한다.

---

## 5. 모니터 표면 기반 입력 처리

### 관련 파일
- `Source/RulesHorror/GameFramework/Pawn/OfficePawn.h`
- `Source/RulesHorror/GameFramework/Pawn/OfficePawn.cpp`
- `Source/RulesHorror/UI/Office/UI_Monitor.h`
- `Source/RulesHorror/UI/Office/UI_Monitor.cpp`
- `Source/RulesHorror/UI/Office/UI_Cursor.h`
- `Source/RulesHorror/UI/Office/UI_Cursor.cpp`

### 구현 내용

`AOfficePawn`은 `UWidgetInteractionComponent`를 기본 Raycast 방식이 아니라 **Custom Hit Result 방식**으로 사용한다.

실행 흐름:

1. 플레이어가 커서로 모니터 표면을 가리킨다.
2. `GetHitResultUnderCursorByChannel()`로 모니터 히트를 가져온다.
3. `BuildCorrectedMonitorWidgetHit()`에서 월드 hit를 위젯 좌표 기준으로 보정한다.
4. 보정된 hit를 `WidgetInteractionComponent::SetCustomHitResult()`로 전달한다.
5. 클릭, 릴리즈, 휠 입력을 내부 UMG 위젯에 전달한다.
6. 별도 모니터 커서 위젯을 갱신해 실제 컴퓨터 같은 피드백을 만든다.

### 입력 보정 알고리즘 핵심

`BuildCorrectedMonitorWidgetHit()`는 다음 과정을 거친다.

- 월드 충돌 지점을 `WidgetComponent` 로컬 좌표로 변환
- 로컬 좌표를 UV 공간으로 변환
- 중심 기준 정규화 값으로 다시 변환
- 모니터 외곽으로 갈수록 입력 오차가 커지는 점을 고려해 반경 기반 축별 보정 수행
- 보정된 UV를 다시 로컬/월드 좌표로 변환
- 최종 hit를 `WidgetInteractionComponent`에 전달

### 기술 포인트

이 파트는 단순 UMG 제작이 아니라, **3D 표면과 2D 입력 시스템을 연결하는 좌표계 처리 문제**를 다룬 구현이다.

특히 다음 점이 이 구현의 핵심이다.

- `WidgetInteractionComponent`를 그대로 쓰지 않고 **커스텀 hit 보정 로직**을 추가했다.
- 실제 사용 시 체감되는 입력 오차를 줄이기 위해 **중심/외곽 보정**을 직접 설계했다.
- 클릭뿐 아니라 **hover, 커서 표시, 스크롤 입력까지** 동일한 경로로 처리했다.

### 핵심 코드

```cpp
void AOfficePawn::DriveWidgetInteraction(float _delta_time)
{
    if (IsInvalid(_InteractingComputer))
        return;

    auto pc = Cast<APlayerController>(GetController());
    if (IsInvalid(pc))
        return;

    FHitResult hit;
    bool is_hit = pc->GetHitResultUnderCursorByChannel(_MonitorScreenWidgetTraceType, true, hit);

    pc->SetShowMouseCursor(!is_hit);

    BuildCorrectedMonitorWidgetHit(hit);
    WidgetInteractionComponent->SetCustomHitResult(hit);
}
```

```cpp
float uv_x = (local_hit_location.Y / draw_size.X) + pivot.X;
float uv_y = pivot.Y - (local_hit_location.Z / draw_size.Y);

const float r2 = FMath::Min((normalized_x * normalized_x) + (normalized_y * normalized_y), 2.0f);

float correction_scale_x = 1.0f - (_MonitorHitCorrectionStrength.X * r2);
float correction_scale_y = 1.0f - (_MonitorHitCorrectionStrength.Y * r2);
```

- 첫 번째 코드는 마우스 hit를 직접 받아 `WidgetInteractionComponent`에 주입하는 흐름이다.
- 두 번째 코드는 월드 충돌 지점을 UV 좌표로 바꾼 뒤, 외곽 입력 오차를 줄이기 위한 반경 기반 보정을 수행하는 핵심 부분이다.

---

## 6. 모니터 UI 상태 관리

### 관련 파일
- `Source/RulesHorror/UI/Office/UI_Monitor.h`
- `Source/RulesHorror/UI/Office/UI_Monitor.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/UI_MonitorScreenWidget.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/UI_MonitorScreenWidget.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/UI_WindowLoading.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/UI_WindowLoading.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/UI_WindowInitUser.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/UI_WindowInitUser.cpp`

### 구현 내용

`UUI_Monitor`는 모니터 내부의 상위 루트 위젯 역할을 한다.

- `WidgetSwitcher`를 이용해 여러 화면 상태를 전환
- 각 화면은 `UUI_MonitorScreenWidget` 기반으로 통일
- 화면 Show/Hide 이벤트를 받아 다음 화면으로 넘기거나 커서를 제어
- 현재 모니터 내부 커서 표시 상태를 관리
- 마지막 활성 화면 인덱스를 저장해 연속성을 유지

초기 부팅 감각을 위한 화면도 분리되어 있다.

- 로딩 화면
- 닉네임 초기 설정 화면
- 이후 윈도우 매니저 기반 메인 화면

### 기술 포인트

- 모니터 하나를 단순 위젯 1장으로 만들지 않고, **“부팅 상태 → 초기 설정 → 메인 OS형 화면”**으로 나누어 관리했다.
- 화면 전환과 커서 표시 요청을 공통 베이스 클래스에 묶어, 개별 화면이 직접 상위 구조를 몰라도 되게 설계했다.

### 핵심 코드

```cpp
void UUI_Monitor::OnShow_Implementation()
{
    Super::OnShow_Implementation();

    ShowMonitorCursor(false, false);

    if (_LastActiveWidgetIndex.IsSet() == false)
    {
        _LastActiveWidgetIndex = 0;
    }

    WidgetSwitcher->SetActiveWidgetIndex(_LastActiveWidgetIndex.GetValue());

    auto actived_screen_widget = Cast<UUI_MonitorScreenWidget>(WidgetSwitcher->GetActiveWidget());
    if (IsValid(actived_screen_widget))
    {
        actived_screen_widget->Show(EWidgetShowType::SelfHitTestInvisible);
    }
}
```

- 모니터 루트 위젯이 내부 화면 상태를 직접 관리한다.
- 마지막 활성 화면 인덱스를 기억해, 단발성 팝업이 아니라 “상태를 가진 장치”처럼 동작하게 만든다.

---

## 7. 멀티 윈도우 데스크톱 UI

### 관련 파일
- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/WindowDefines.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/WindowBase.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/WindowBase.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/UI_WindowManager.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/WindowBase/UI_WindowManager.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/Window_Explorer.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/Window_Explorer.cpp`

### 구현 내용

이 시스템은 단일 메뉴 화면이 아니라, **아이콘-창-탭 구조를 가진 OS형 UI**를 구현했다.

#### `UWindowBase`
- 개별 창의 공통 베이스
- 창 포커스 처리
- 최소화 / 복구 / 닫기
- 창 이동 / 리사이즈
- 최대화 상태와 일반 상태 전환

#### `UUI_WindowManager`
- 아이콘 더블클릭 시 창 생성
- 창 탭 생성 및 탭 클릭 동작 관리
- 최상단 창 ZOrder 관리
- 창이 닫히거나 최소화될 때 상태 업데이트
- 새 창 오픈 시 이전 창 위치를 기준으로 오프셋 배치

#### `EWindowCommand`
- `Minimize`, `RestoreSize`, `Close`, `Move`, `Resize_*`, `EndDrag` 등 창 조작 명령을 정의

### 기술 포인트

- 게임 UI를 일반적인 풀스크린 메뉴 대신, **데스크톱/브라우저 사용 경험에 가까운 창 시스템**으로 구현했다.
- 창 이동/리사이즈는 위젯 애니메이션이 아니라, **캔버스 슬롯의 위치/크기를 직접 갱신하는 방식**으로 처리했다.
- 최대화 상태에서 드래그할 때 일반 창으로 복원되는 처리까지 구현해, 실제 OS UX에 가까운 동작을 재현했다.

### 핵심 코드

```cpp
void UUI_WindowManager::OpenWindow(EWindowWidgetType _type)
{
    auto data_ptr = _WindowDataMap.Find(_type);
    if (IsInvalid(data_ptr))
        return;

    if (IsInvalid(data_ptr->WindowWidget))
    {
        if (CreateWindowWidget(_type, *data_ptr) == false)
            return;
    }

    auto window_widget = data_ptr->WindowWidget;

    if (IsInvalid(window_widget->GetParent()))
    {
        auto cp_slot = CP_Window->AddChildToCanvas(window_widget);
        if (IsValid(cp_slot))
        {
            cp_slot->SetSize(_DefaultWindowSize);
        }
    }

    window_widget->Show(EWidgetShowType::SelfHitTestInvisible);
    SetTopWindow(window_widget);
}
```

```cpp
if (_DragType == EWindowDragType::Move)
{
    auto cp_slot = Cast<UCanvasPanelSlot>(Slot);
    if (IsValid(cp_slot))
    {
        cp_slot->SetPosition(_InitialWindowPos + drag_delta);
    }
}
```

- 첫 번째 코드는 창 생성/재사용/표시/최상단 배치를 담당한다.
- 두 번째 코드는 드래그 시 캔버스 슬롯 위치를 직접 갱신하는 핵심 로직이다.

---

## 8. 사이트 및 스토리 콘텐츠 연결

### 관련 파일
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/UI_SiteBase.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/UI_SiteBase.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/UI_SitePanel.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/UI_SitePanel.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/StoryListSite/Site_StoryList.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/StoryListSite/Site_StoryList.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/StoryDetailSite/Site_StoryDetail.h`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/StoryDetailSite/Site_StoryDetail.cpp`
- `Source/RulesHorror/Item/RulesHorrorItemDefines.h`
- `Source/RulesHorror/Item/RulesHorrorItemHelper.h`
- `Source/RulesHorror/Item/RulesHorrorItemHelper.cpp`

### 구현 내용

모니터 안의 브라우저형 UI는 정적인 화면이 아니라, **데이터 기반으로 스토리 목록과 상세 정보를 표시**한다.

#### `FStoryTableRow`
- `AdditionalSiteAddress`
- `StoryMap`

#### `URulesHorrorItemHelper`
- 스토리 ID로 Row 조회
- 전체 스토리 Row 목록 조회

#### `USite_StoryList`
- 전체 스토리 Row를 페이지 단위로 나누어 표시
- 라디오 버튼 그룹으로 페이지 전환
- 스토리 제목 버튼 클릭 시 상세 페이지 진입

#### `USite_StoryDetail`
- 선택된 스토리의 제목과 추가 사이트 주소 표시
- 연결된 `StoryMap`으로 레벨 이동 가능

#### `UUI_SitePanel`
- 목록/상세 사이트 전환
- 주소창 문자열 브로드캐스트
- 저장된 닉네임 불러오기

### 기술 포인트

- 스토리 정보, 사이트 주소, 연결 맵을 코드에 박아 넣지 않고 **DataTable 기반**으로 연결했다.
- 따라서 UI 로직을 거의 건드리지 않고도 스토리 항목 추가/수정이 가능하다.
- 이 구조는 이후 StoryFlow, 해금 상태, 저장 데이터와도 자연스럽게 연결될 수 있다.

### 핵심 코드

```cpp
void USite_StoryDetail::SetStoryID(FItemID_Story _story_id)
{
    if (_CurrentStoryID == _story_id)
        return;

    _CurrentStoryID = _story_id;
    _LastStoryID = _story_id;

    const auto& story_item_row = URulesHorrorItemHelper::GetStoryItemRow(_CurrentStoryID);

    SetAdditionalSiteAddress(story_item_row.AdditionalSiteAddress);
    SetTitle(story_item_row.GetDisplayName());
}
```

```cpp
void USite_StoryDetail::PlayCurrentStory()
{
    if (_CurrentStoryID.IsValid() == false)
        return;

    const auto& story_item_row = URulesHorrorItemHelper::GetStoryItemRow(_CurrentStoryID);

    if (story_item_row.StoryMap.IsNull())
        return;

    UGameplayStatics::OpenLevelBySoftObjectPtr(this, story_item_row.StoryMap);
}
```

- 선택한 스토리 ID를 DataTable Row로 해석해 주소/제목/UI를 갱신한다.
- 최종적으로는 Row에 연결된 `StoryMap`을 열기 때문에, 콘텐츠 확장이 데이터 중심으로 이뤄진다.

---

## 9. 저장 데이터와의 연결

### 관련 파일
- `Source/RulesHorror/UI/Office/MonitorScreen/UI_WindowInitUser.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/UI_SitePanel.cpp`
- `Source/RulesHorror/UI/Office/MonitorScreen/Explorer/StoryListSite/Site_StoryList.cpp`

### 구현 내용

저장 시스템은 현재 일부만 연결되어 있지만, 구조상 컴퓨터 UI와 이어질 준비가 되어 있다.

- 초기 사용자 닉네임 저장/불러오기
- 사이트 화면에서 닉네임 표시
- 스토리 리스트 쪽에 진행도 반영 TODO 존재

### 기술 포인트

- 컴퓨터 UI가 단발성 연출이 아니라, **저장 데이터 기반 상태 변화가 가능한 허브 시스템**으로 설계되어 있다.
- 현재는 닉네임 저장이 연결되어 있고, 향후 해금/클리어/낙인 상태 반영으로 확장될 여지가 남아 있다.

---

## 10. 기술적으로 강조할 수 있는 구현 포인트

### 1) 디제틱 UI 설계
- 월드 오브젝트를 통해 UI에 진입하는 구조
- 메뉴를 게임 바깥 레이어가 아니라 세계 안에 통합

### 2) 3D 표면 ↔ 2D UMG 입력 연결
- `WidgetInteractionComponent`를 커스터마이즈해 표면 입력 보정 구현
- 모니터 외곽에서 생기는 조작 오차를 줄이는 좌표 보정 로직 설계

### 3) 상호작용과 연출의 결합
- 상호작용 직후 즉시 UI 전환이 아니라, 이동 포인트를 활용해 컴퓨터 앞에 앉는 흐름을 구현
- 플레이어 시점과 UI 상태 전환을 자연스럽게 묶음

### 4) 멀티 윈도우 UI 프레임 설계
- 창 생성, 포커스, 드래그, 리사이즈, 탭, 최소화, 복구를 공통 베이스로 분리
- 이후 다른 앱/사이트 UI를 추가하기 쉬운 구조

### 5) 데이터 기반 콘텐츠 확장성
- 스토리 사이트 정보와 맵 이동 대상을 DataTable로 관리
- 기획 변경에 대한 대응력이 좋음

---

## 11. 한계와 개선 예정 지점

현재 기준으로 확인되는 미완성 또는 확장 예정 지점은 다음과 같다.

- `USite_StoryList`에 스토리 진행도/해금 상태 반영 TODO 존재
- 창 종류는 구조상 확장 가능하지만 현재는 `Explorer` 중심
- 컴퓨터 종료 시 전원 Off 연출과 후처리는 Blueprint 또는 추가 연동 여지가 있음
- 허브 변화, 낙인 상태, StoryFlow 진행 조건과의 연결은 문서상 추가 확장 예정

이 문서는 완성된 제품 소개보다는, **이미 구현된 기술 기반과 구조적 강점**을 설명하는 데 초점을 둔다.

---

## 12. 기술 요약 문장

이 시스템은 Unreal Engine 5에서 **월드 상호작용 Actor, 이동 포인트 연출, `WidgetInteractionComponent` 기반 입력 보정, 멀티 윈도우 UMG 구조, DataTable 기반 콘텐츠 연결**을 결합해 구현한 디제틱 컴퓨터 UX다. 특히 3D 모니터 표면과 2D UI 입력을 자연스럽게 이어주기 위해 **커스텀 hit 보정 로직**을 설계했고, 브라우저/데스크톱 형태의 내부 UI를 통해 스토리 선택 허브로 확장 가능한 구조를 만들었다.


