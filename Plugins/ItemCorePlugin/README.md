# ItemCore

`ItemCore`는 Unreal Engine 5 프로젝트에서 **ItemID**, **Item DataTable Row**, **Item Registry**, 그리고 에디터용 ItemID 선택 UI를 제공하는 아이템 식별/조회 기반 플러그인이다.

이 문서는 `Plugins/ItemCorePlugin`의 **현재 코드 기준**으로 유지되는 기술 README다. 실제 사용 절차 중심 문서는 [UserGuide.md](./UserGuide.md)를 참고한다.

---

## 핵심 요약

- `FItemID`는 `Type 3자리 + SubType 3자리 + Serial 4자리` 형식의 10진수 기반 아이템 식별자다.
- `EItemType`은 아이템 대분류를 정의한다.
- SubType은 ItemType별 enum으로 확장할 수 있고, 기본값은 `EDefaultItemSubType`이다.
- `FItemTableRow`는 Item DataTable Row의 기본 구조체다.
- 실제 프로젝트 Row는 `FItemTableRow`를 상속해 추가 필드를 붙인다.
- `UItemRegistryDataAsset`은 Registry에 포함할 Item DataTable 목록이다.
- `UItemDeveloperSettings::_ItemRegistryDataAsset`에 Registry Data Asset을 지정한다.
- `UItemRegistrySubsystem`은 Data Asset에 등록된 테이블만 인덱싱한다.
- `UItemHelper` / `UItemIDHelper`는 Blueprint/C++ 조회와 ItemID 조작 API를 제공한다.
- `ItemCoreEditor`는 `FItemID` Details 커스터마이징, ItemID picker, `Make ItemID` Blueprint 노드, Refresh Registry 툴바 버튼을 제공한다.
- `ToggleShowItemID` 콘솔 명령으로 비 Shipping 빌드에서 DisplayName에 ItemID 표시를 토글할 수 있다.

---

## 플러그인 정보

| 항목 | 값 |
| --- | --- |
| 경로 | `Plugins/ItemCorePlugin` |
| 플러그인 이름 | `ItemCore` |
| 모듈 | `ItemCore`, `ItemCoreEditor` |
| Engine version | 5.7 |
| Content 포함 | 있음 (`CanContainContent: true`) |
| 의존 플러그인 | `CommonLibrary` |

---

## 모듈 구성

| 모듈 | 타입 | 역할 |
| --- | --- | --- |
| `ItemCore` | Runtime | ItemID, ItemTableRow, Registry DataAsset, Registry Subsystem, Blueprint Helper |
| `ItemCoreEditor` | Editor | ItemID Details 커스터마이징, K2 노드, 툴바 버튼, 에디터 스타일 |

### 주요 의존성

`ItemCore`:

- `Core`, `CoreUObject`, `Engine`
- `DeveloperSettings`
- `CommonLibrary`
- `Slate`, `SlateCore`(Private)

`ItemCoreEditor`:

- `ItemCore`
- `PropertyEditor`
- `BlueprintGraph`
- `UnrealEd`
- `ToolMenus`
- `KismetCompiler`
- `MessageLog`
- `Projects`
- `CommonLibrary`

---

## 주요 런타임 타입

### `EItemType`

파일: `Source/ItemCore/Public/ItemDefines.h`

아이템 대분류 enum이다.

기본/예약 값:

| 값 | 의미 |
| --- | --- |
| `NA` | 미사용/invalid 값. Hidden |
| `MAX` | enum range 끝. Hidden |

프로젝트별 아이템 타입은 이 enum에 추가한다. 이 문서의 예제에서는 샘플 Type으로 `EItemType::Story`를 사용한다. `Story`는 ItemCore가 강제하는 내장 타입이 아니라 샘플 예제명이며, 실제 프로젝트에서는 동일한 샘플 Type을 추가하거나 프로젝트에 맞는 Type 이름으로 바꿔 사용한다.

`ENUM_RANGE_BY_COUNT(EItemType, EItemType::MAX)`가 적용되어 에디터 커스터마이징에서 usable type 목록 생성에 사용된다. 프로젝트 Type은 `NA`와 `MAX` 사이에 둔다.

---

### `EDefaultItemSubType`

파일: `Source/ItemCore/Public/ItemDefines.h`

별도 SubType enum을 연결하지 않은 ItemType이 기본으로 사용하는 enum이다.

| 값 | 의미 |
| --- | --- |
| `NA` | invalid 값. Hidden |
| `Default` | 기본 SubType |

새 ItemType별 SubType enum을 추가하려면 `FItemID::GetSubTypeEnum(EItemType)` switch에서 해당 enum을 반환하도록 확장한다.

---

### `FItemID`

파일: `Source/ItemCore/Public/ItemID.h`, `Private/ItemID.cpp`

아이템 고유 식별자다.

포맷:

```text
Type 3자리 + SubType 3자리 + Serial 4자리
000 000 0000
```

중요: bit packing이 아니라 10진수 값 조합이다.

내부 값:

```cpp
UPROPERTY(EditAnywhere)
uint32 Value = 0;
```

주요 상수:

| 상수 | 값 | 의미 |
| --- | --- | --- |
| `TypeMultiplier` | `10000000` | Type 자리 이동 단위 |
| `SubTypeMultiplier` | `10000` | SubType 자리 이동 단위 |
| `MaxTypeValue` | `255` | Type 최대값 |
| `MaxSubTypeValue` | `255` | SubType 최대값 |
| `MaxSerialValue` | `9999` | Serial 최대값 |

주요 API:

| 함수 | 의미 |
| --- | --- |
| `Set(EItemType, uint8, uint16)` | Type/SubType/Serial로 값 설정 |
| `SetType(EItemType)` | Type 설정 후 SubType/Serial invalidate |
| `SetSubType(uint8)` | SubType 설정 후 Serial invalidate |
| `SetSerial(uint16)` | Serial 설정 |
| `GetType()` | Type 반환 |
| `GetRawSubType()` | 3자리 raw SubType 반환 |
| `GetSubType()` | `uint8` SubType 반환 |
| `GetSerial()` | Serial 반환 |
| `Validate()` | ItemID 유효성 검사 |
| `IsValid()` | `Validate().IsValid()` |
| `ToString()` | 내부 정수 문자열 반환 |

Validation 정책:

- Type은 UEnum에 존재해야 한다.
- Type은 `NA` / `MAX`가 아니어야 한다.
- raw SubType은 `MaxSubTypeValue` 이하이어야 한다.
- SubType은 0이 아니어야 한다.
- SubType은 Type에 대응하는 enum의 usable 값이어야 한다.
- Serial은 0이 아니어야 한다.
- Serial은 `MaxSerialValue` 이하이어야 한다.

---

### `FItemIDValidationResult`

파일: `Source/ItemCore/Public/ItemDefines.h`

`FItemID::Validate()` 결과다.

| 필드/함수 | 의미 |
| --- | --- |
| `Reason` | 실패/성공 사유 문자열 |
| `IsValid()` | valid 여부 |
| `IsInvalid()` | invalid 여부 |

---

### `FItemTableReference`

파일: `Source/ItemCore/Public/ItemDefines.h`

Item Registry에 등록할 DataTable을 감싸는 참조 구조체다. 일반 DataTable 참조와 구분하기 위해 `_ItemTables`는 이 타입을 사용한다.

```cpp
USTRUCT(BlueprintType)
struct ITEMCORE_API FItemTableReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UDataTable> DataTable = nullptr;
};
```

에디터 모듈의 `FItemTableReferenceCustomization`은 picker에서 `FItemTableRow` 파생 RowStruct를 가진 DataTable만 표시한다.

---

### `FItemTableRow`

파일: `Source/ItemCore/Public/ItemTableRow.h`, `Private/ItemTableRow.cpp`

Item DataTable의 기본 Row 구조체다. 프로젝트별 Item Row는 이 구조체를 상속한다.

주요 필드:

| 필드 | 의미 |
| --- | --- |
| `ItemID` | 아이템 식별자 |
| `DisplayName` | 표시 이름 |
| `DevState` | 개발/출시 상태 |

`EItemDevelopmentState`:

| 값 | 의미 |
| --- | --- |
| `NotUsed` | 사용하지 않음, 샘플 등 |
| `Developing` | 개발 중 |
| `Ready` | 출시 가능 |
| `Shipping` | Shipping |

주요 API:

| 함수 | 의미 |
| --- | --- |
| `IsUsableItem()` | 현재 빌드에서 사용 가능한 아이템인지 반환 |
| `GetDisplayName()` | 표시 이름 반환. debug 표시 옵션이 켜져 있으면 ItemID 포함 |
| `ToggleShowItemIDOnDisplayName()` | 비 Shipping 빌드에서 DisplayName ItemID 표시 토글 |

빌드별 `IsUsableItem()` 정책:

- Shipping: `DevState == Shipping`만 true
- Non-Shipping: `Developing`, `Ready`, `Shipping` true
- `NotUsed`는 항상 false

에디터 동작:

- DataTable 변경 시 `HandleItemIDChanged()`를 통해 Registry refresh를 요청한다.
- `IsDataValid()`에서 ItemID 유효성을 검사한다.
- `TableItemType`이 설정된 파생 Row는 다른 Type의 ItemID를 거부한다.

---

### `UItemRegistryDataAsset`

파일: `Source/ItemCore/Public/ItemRegistryDataAsset.h`

Item Registry가 인덱싱할 DataTable 목록을 가진 `UPrimaryDataAsset`이다.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
TArray<FItemTableReference> _ItemTables;
```

정책:

- Item Registry의 source of truth다.
- Runtime path scan은 사용하지 않는다.
- `_ItemTables`에 등록된 DataTable만 Registry 대상이다.
- DataTable RowStruct는 `FItemTableRow` 계열이어야 한다.
- Details picker는 `Source/ItemCoreEditor/Private/ItemTableReferenceCustomization.cpp`의 `FItemTableReference` 커스터마이징으로 `FItemTableRow` 파생 RowStruct만 표시한다.
- 런타임 등록 시에도 `UItemRegistrySubsystem::IsSupportedItemTable()`로 다시 검사한다.

Primary Asset ID:

```cpp
FPrimaryAssetId(TEXT("ItemRegistry"), GetFName())
```

---

### `UItemDeveloperSettings`

파일: `Source/ItemCore/Public/ItemDeveloperSettings.h`

Project Settings에 저장되는 전역 ItemCore 설정이다.

| 설정 | 의미 |
| --- | --- |
| `_ItemRegistryDataAsset` | Registry에 사용할 `UItemRegistryDataAsset` soft reference |

현재는 고정 경로나 자동 scan path를 사용하지 않는다. 사용자가 Project Settings에서 직접 DataAsset을 지정한다.

---

### `UItemRegistrySubsystem`

파일: `Source/ItemCore/Public/ItemRegistrySubsystem.h`, `Private/ItemRegistrySubsystem.cpp`

Engine Subsystem으로, ItemID에서 DataTable Row를 찾기 위한 런타임 인덱스를 관리한다.

주요 내부 데이터:

| 데이터 | 의미 |
| --- | --- |
| `_RegisteredItemTables` | 이번 refresh에서 인덱싱할 DataTable 목록 |
| `_ItemTypeIndexMap` | `EItemType -> ItemID -> RowReference` 인덱스 |

주요 public API:

| 함수 | 의미 |
| --- | --- |
| `RefreshRegistry()` | DeveloperSettings의 Registry DataAsset 기준으로 전체 Registry 재빌드 |
| `RefreshItemTable(const UDataTable*)` | 테이블 변경 이벤트용. 현재는 전체 `RefreshRegistry()`로 위임 |
| `Contains(const FItemID&)` | ItemID 등록 여부 |
| `GetItemCount()` | 전체 등록 아이템 수 |
| `GetTypeItemCount(EItemType)` | Type별 등록 아이템 수 |
| `FindItemRow<T>(const FItemID&)` | 타입 안전 Row 조회 |
| `GetItemRowsByType<T>(EItemType, bool)` | Type별 Row 목록 조회 |

Refresh 흐름:

1. Message Log clear
2. `_RegisteredItemTables` / `_ItemTypeIndexMap` 초기화
3. `UItemDeveloperSettings::_ItemRegistryDataAsset` 동기 로드
4. `_ItemTables`의 DataTable을 등록
5. 각 DataTable Row를 ItemID 기준으로 인덱싱
6. duplicate ItemID / invalid ItemID / unsupported RowStruct를 Message Log에 보고
7. 성공/실패 Notification 표시

중요 정책:

- DataAsset에 등록되지 않은 테이블은 Registry 대상이 아니다.
- `RefreshItemTable()`도 임의 테이블을 직접 등록하지 않고 전체 refresh만 수행한다.
- duplicate ItemID가 있으면 해당 Row는 건너뛰고 실패 상태를 반환한다.

---

### `UItemRegistryValidationSubsystem`

파일: `Source/ItemCore/Public/ItemRegistryValidationSubsystem.h`, `Private/ItemRegistryValidationSubsystem.cpp`

`UGameInstanceSubsystem`이다. GameInstance 초기화 시 Item Registry refresh를 수행한다.

동작:

- `GEngine->GetEngineSubsystem<UItemRegistrySubsystem>()` 조회
- `RefreshRegistry()` 호출
- 에디터에서는 실패 시 popup 표시
- 비에디터에서는 실패 시 `UE_LOG(LogTemp, Error, ...)` 출력

---

### `UItemHelper`

파일: `Source/ItemCore/Public/ItemHelper.h`, `Private/ItemHelper.cpp`

Blueprint/C++에서 Item Row를 조회하는 헬퍼다.

C++ 템플릿:

```cpp
const T* UItemHelper::FindItemRow<T>(FItemID _item_id);
TArray<const T*> UItemHelper::GetAllItemRowsByType<T>(EItemType _item_type, bool _usable_item_only = true);
```

Blueprint 함수:

```cpp
static bool FindItemRow(FItemID _item_id, FItemTableRow& _out_item_row);
```

조회는 `GEngine->GetEngineSubsystem<UItemRegistrySubsystem>()`를 통해 수행한다.

---

### `UItemIDHelper`

파일: `Source/ItemCore/Public/ItemIDHelper.h`, `Private/ItemIDHelper.cpp`

Blueprint에서 ItemID를 만들고 분해하기 위한 함수 모음이다.

| 함수 | 의미 |
| --- | --- |
| `MakeItemID` | Type/SubType/Serial로 ItemID 생성. BlueprintInternalUseOnly |
| `BreakItemID` | ItemID를 Type/SubType/Serial로 분해 |
| `GetType` | Type 반환 |
| `GetSubType` | SubType 반환 |
| `GetSerial` | Serial 반환 |
| `ToInteger` | 정수 값 반환 |
| `ToString` | 문자열 반환 |
| `IsValid` | ItemID 유효성 및 reason 반환 |

`MakeItemID`는 Serial을 `0~9999`로 clamp한다.

---

## 주요 에디터 기능

### `FItemIDCustomization`

파일: `Source/ItemCoreEditor/Public/ItemIDCustomization.h`, `Private/ItemIDCustomization.cpp`

`FItemID` Details UI를 Type/SubType/Serial 입력 UI로 커스터마이징한다.

제공 UI:

- Type ComboBox
- Type별 SubType ComboBox
- Serial NumericEntryBox
- Validation 상태 표시 점
- 현재 ItemID 숫자 표시
- Registry 기반 ItemID picker 버튼

동작:

- Type 변경 시 SubType/Serial을 invalidate한다.
- SubType 변경 시 Serial을 invalidate한다.
- Serial 변경 시 ItemID 값을 갱신한다.
- picker는 현재 Type과 선택 가능한 SubType 기준으로 Registry Row를 나열한다.
- picker 항목은 `ItemID (DisplayName)` 형식으로 표시한다.

---

### `UK2Node_MakeItemID`

파일: `Source/ItemCoreEditor/Public/K2Node_MakeItemID.h`, `Private/K2Node_MakeItemID.cpp`

Blueprint에서 사용하는 pure `Make ItemID` 노드다.

입력 핀:

| 핀 | 의미 |
| --- | --- |
| `Type` | `EItemType` |
| `SubType` | 선택된 Type에 대응하는 SubType enum 또는 byte |
| `Serial` | int |

출력 핀:

| 핀 | 의미 |
| --- | --- |
| `ItemID` | `FItemID` |

Compile 시 `UItemIDHelper::MakeItemID` 호출 노드로 확장된다.

---

### Toolbar: Refresh Registry

파일: `Source/ItemCoreEditor/Private/ItemCoreEditor.cpp`

Level Editor toolbar에 `Refresh Registry` 버튼을 추가한다.

동작:

```text
Click -> GEngine->GetEngineSubsystem<UItemRegistrySubsystem>() -> RefreshRegistry()
```

Message Log 이름은 `ItemRegistry`이며, `ItemCoreEditor` 시작 시 `Item Registry` Log Listing을 등록한다.

---

### Console Command

파일: `Source/ItemCore/Private/ItemCore.cpp`

비 Shipping 빌드에서 다음 콘솔 명령을 등록한다.

```text
ToggleShowItemID
```

동작:

- `FItemTableRow::DEBUG_ShowItemIDOnDisplayName` 토글
- `FItemTableRow::GetDisplayName()` 반환 텍스트에 `[ItemID] DisplayName` 표시

---

## 프로젝트 확장 예

프로젝트별 Item Row는 `FItemTableRow`를 상속해 필요한 필드를 추가한다. 생성자에서 Type을 지정하면 잘못된 ItemID 입력을 줄일 수 있다.

```cpp
USTRUCT(BlueprintType)
struct MYGAME_API FMyItemTableRow : public FItemTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UObject> RelatedAsset;

public:
	FMyItemTableRow()
		: FItemTableRow(EItemType::Story)
	{
	}
};
```

Type 전용 ItemID wrapper도 만들 수 있다.

```cpp
USTRUCT(BlueprintType)
struct MYGAME_API FItemID_Story : public FItemID
{
	GENERATED_BODY()

public:
	FItemID_Story() noexcept
		: FItemID(EItemType::Story, 0, 0)
	{
	}

	FItemID_Story(const FItemID& _other)
		: FItemID(_other)
	{
	}
};
```

---

## Content / Sample

플러그인은 `CanContainContent=true`이며 샘플 DataTable을 포함한다.

| 경로 | 내용 |
| --- | --- |
| `Content/Sample/DT_ItemSample.uasset` | Item DataTable 샘플 |
| `Content/Sample/DT_ItemSample2.uasset` | Item DataTable 샘플 |
| `Resources/Icon128.png` | 플러그인 아이콘 |
| `Resources/ItemRegistryRefresh_40.png` | Refresh Registry 툴바 아이콘 |

---

## 현재 한계 / 주의사항

- `_ItemRegistryDataAsset`은 Project Settings에서 직접 지정해야 한다.
- 고정 Content 경로나 자동 DataTable scan은 사용하지 않는다.
- 패키징 cook 포함 여부는 프로젝트 패키징 설정/참조 정책에 맞게 확인해야 한다.
- `_ItemTables` picker의 `FItemTableReference` 필터가 프로젝트 파생 RowStruct를 잘 보여주는지 에디터에서 확인해야 한다.
- `RefreshRegistry()` 실패 시 일부 index가 남을 수 있는 설계는 현재 보류 상태다.
- `FItemID`는 10진수 조합 방식이므로 자리수 정책을 바꿀 때 기존 DataTable/Save 데이터 호환성을 고려해야 한다.
- `ToggleShowItemID`는 Shipping 빌드에서 등록되지 않는다.

---

## 추천 코드 읽기 순서

1. `ItemCore.uplugin`
2. `Source/ItemCore/ItemCore.Build.cs`
3. `Source/ItemCore/Public/ItemDefines.h`
4. `Source/ItemCore/Public/ItemID.h`
5. `Source/ItemCore/Private/ItemID.cpp`
6. `Source/ItemCore/Public/ItemTableRow.h`
7. `Source/ItemCore/Private/ItemTableRow.cpp`
8. `Source/ItemCore/Public/ItemRegistryDataAsset.h`
9. `Source/ItemCore/Public/ItemDeveloperSettings.h`
10. `Source/ItemCore/Public/ItemRegistrySubsystem.h`
11. `Source/ItemCore/Private/ItemRegistrySubsystem.cpp`
12. `Source/ItemCore/Public/ItemHelper.h`
13. `Source/ItemCore/Public/ItemIDHelper.h`
14. `Source/ItemCoreEditor/Private/ItemTableReferenceCustomization.cpp`
15. `Source/ItemCoreEditor/Private/ItemIDCustomization.cpp`
16. `Source/ItemCoreEditor/Private/K2Node_MakeItemID.cpp`
17. `Source/ItemCoreEditor/Private/ItemCoreEditor.cpp`

---

## 관련 문서

- [User Guide](./UserGuide.md) — 플러그인을 사용하는 디자이너/개발자용 작업 가이드
