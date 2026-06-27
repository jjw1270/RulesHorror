# ItemCore User Guide

이 문서는 ItemCore 플러그인을 실제 프로젝트에서 사용하는 사람을 위한 작업 가이드다. 코드 구조 설명보다 **무엇을 만들고, 어디에 설정하고, 어떻게 조회/검증하는지**에 초점을 둔다.

---

## 1. 기본 개념

ItemCore는 다음 단위로 아이템 데이터를 구성한다.

| 이름 | 설명 |
| --- | --- |
| ItemID | Type/SubType/Serial로 구성된 고유 아이템 번호 |
| ItemType | 아이템 대분류. `EItemType` enum |
| SubType | ItemType 안의 세부 분류. Type별 enum으로 확장 가능 |
| ItemTableRow | Item DataTable Row 기본 구조체 |
| ItemRegistryDataAsset | Registry가 읽을 Item DataTable 목록 |
| ItemRegistrySubsystem | ItemID로 DataTable Row를 찾는 Engine Subsystem |
| ItemHelper | Blueprint/C++ 조회 헬퍼 |

가장 흔한 흐름은 다음과 같다.

```text
Item Row Struct 작성
-> Item DataTable 생성
-> ItemRegistryDataAsset에 DataTable 등록
-> Project Settings에 Registry DataAsset 지정
-> Refresh Registry
-> ItemID로 Row 조회
```

---

## 2. 처음 설정하기

### 2.1 ItemRegistryDataAsset 만들기

1. Content Browser에서 `Data Asset`을 생성한다.
2. 클래스 선택 창에서 `ItemRegistryDataAsset`을 선택한다.
3. 원하는 위치에 저장한다. 경로는 자유다.
4. `_ItemTables` 배열에 Item DataTable을 추가한다.

주의:

- `_ItemTables` picker에는 `FItemTableRow` 계열 RowStruct를 가진 DataTable만 표시된다.
- 내부적으로 `_ItemTables`는 `FItemTableReference` 배열이며, 각 entry의 `DataTable`에 테이블을 지정한다.
- 일반 `FTableRowBase` DataTable은 Item Registry 대상이 아니다.
- DataAsset 위치는 플러그인 코드에서 강제하지 않는다.

### 2.2 Project Settings 설정

Project Settings에서 Item 설정을 찾고 다음을 지정한다.

| 설정 | 필수 | 설명 |
| --- | --- | --- |
| ItemRegistryDataAsset | 필수 | 방금 만든 `ItemRegistryDataAsset` |

설정은 다음 C++ 필드에 저장된다.

```cpp
UItemDeveloperSettings::_ItemRegistryDataAsset
```

### 2.3 Registry 갱신

에디터 상단 툴바에서 `Refresh Registry` 버튼을 누른다.

성공하면 Message Log에 다음 형태로 표시된다.

```text
Refresh Registry...
Register=Success, BuildIndex=Success, RegisteredTables=N, IndexedItems=N
Refresh Item Registry Success!
```

실패하면 Message Log에서 unsupported RowStruct, invalid ItemID, duplicate ItemID 등을 확인한다.

---

## 3. ItemID 규칙

ItemID는 10진수 자리 조합이다.

```text
Type 3자리 + SubType 3자리 + Serial 4자리
000 000 0000
```

예시:

```text
001 001 0001
```

의미:

| 구간 | 값 | 의미 |
| --- | --- | --- |
| Type | `001` | 프로젝트가 `EItemType`에 추가한 대분류 |
| SubType | `001` | Type 안의 세부 분류 |
| Serial | `0001` | 같은 Type/SubType 안의 번호 |

유효성 기준:

- Type은 `NA`나 `MAX`가 아니어야 한다.
- Type은 현재 255 이하 값만 사용한다. 표기는 3자리 구간이지만 `000~999` 전체를 허용하는 구조가 아니다.
- SubType은 0이 아니어야 한다.
- SubType은 해당 Type의 SubType enum에 존재해야 한다.
- SubType은 현재 255 이하 값만 사용한다. 표기는 3자리 구간이지만 `000~999` 전체를 허용하는 구조가 아니다.
- Serial은 0이 아니어야 한다.
- Serial은 9999 이하이어야 한다.

주의:

- ItemID를 직접 정수로 입력하거나 CSV/저장 데이터에서 가져오는 경우에도 반드시 유효성 검사를 통과해야 한다.
- C++에서는 가능하면 raw 정수 값을 직접 넣기보다 Type/SubType/Serial을 분리해 `Set()` 또는 `UItemIDHelper::MakeItemID()` 경로로 생성한다.

---

## 4. Item Row Struct 만들기

프로젝트별 아이템 데이터는 `FItemTableRow`를 상속해서 만든다.

예:

```cpp
USTRUCT(BlueprintType)
struct MYGAME_API FMyItemTableRow : public FItemTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UObject> RelatedAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StackSize = 1;

public:
	FMyItemTableRow()
		: FItemTableRow(EItemType::MyItemType)
	{
	}
};
```

생성자에서 `FItemTableRow(EItemType::MyItemType)`를 호출하면 Row의 Type을 프로젝트 전용 Type으로 고정하는 데 도움이 된다. `EItemType::MyItemType`은 예시 이름이며, 실제 프로젝트에서는 먼저 `EItemType`에 필요한 Type을 추가한 뒤 사용한다.

주의:

- RowStruct는 반드시 `FItemTableRow` 계열이어야 한다.
- `ItemID`는 Row마다 고유해야 한다.
- 같은 Registry 안에서 duplicate ItemID가 있으면 Registry refresh가 실패한다.

---

## 5. Item DataTable 만들기

1. Content Browser에서 DataTable을 생성한다.
2. RowStruct로 프로젝트 Item Row를 선택한다.
   - 예: `FMyItemTableRow`
3. Row를 추가한다.
4. 각 Row의 `ItemID`, `DisplayName`, `Description`, `DevState`를 설정한다.

기본 필드:

| 필드 | 설명 |
| --- | --- |
| ItemID | 고유 식별자 |
| DisplayName | UI 표시 이름 |
| Description | UI/툴팁 등에 사용할 아이템 설명 |
| DevState | 개발/출시 상태 |
| DevComment | 에디터 전용 개발 메모. 패키징 런타임 데이터로 쓰지 않는다 |

`DevState` 추천 사용:

| 상태 | 사용 기준 |
| --- | --- |
| NotUsed | 샘플, 폐기, 미사용 데이터 |
| Developing | 개발 중 데이터 |
| Ready | 출시 가능 데이터 |
| Shipping | Shipping 빌드 포함 데이터 |

Shipping 빌드에서는 `Shipping` 상태인 Row만 usable item으로 취급된다.

---

## 6. ItemRegistryDataAsset에 등록하기

`ItemRegistryDataAsset`을 열고 `_ItemTables`에 DataTable을 추가한다.

권장:

- 실제 게임에서 조회할 Item DataTable만 등록한다.
- picker에 보이지 않는 테이블은 RowStruct가 `FItemTableRow`를 상속하는지 확인한다.
- 테스트/샘플 테이블은 필요할 때만 등록한다.
- 여러 테이블에 같은 ItemID가 없도록 관리한다.

중요:

- Registry는 `_ItemTables`에 등록된 테이블만 인덱싱한다.
- 에디터에서 다른 Item DataTable을 수정해도 DataAsset에 없으면 Registry 대상이 아니다.
- `RefreshItemTable()`도 전체 Registry refresh로 위임되므로 DataAsset이 source of truth다.

---

## 7. FItemID 편집하기

`FItemID`는 Details 패널에서 전용 UI로 표시된다.

구성:

| UI | 설명 |
| --- | --- |
| Type ComboBox | `EItemType` 선택 |
| SubType ComboBox | Type에 대응하는 SubType 선택 |
| Serial 입력 | 0~9999 범위 숫자 입력 |
| 상태 점 | ItemID 유효성 표시 |
| 숫자 표시 | 실제 ItemID 정수 값 |
| Picker 버튼 | Registry에 등록된 ItemID에서 선택 |

작업 순서:

1. Type을 선택한다.
2. SubType을 선택한다.
3. Serial을 입력한다.
4. 상태 점 tooltip으로 유효성 메시지를 확인한다.

주의:

- Type을 바꾸면 SubType/Serial은 invalidate된다.
- SubType을 바꾸면 Serial은 invalidate된다.
- Serial 0은 invalid다.

---

## 8. Registry Picker 사용하기

`FItemID` Details UI의 Picker 버튼은 현재 Registry에서 ItemID를 가져온다.

동작:

- 현재 Type이 usable하면 버튼이 활성화된다.
- 현재 SubType이 valid면 해당 SubType Row만 보여준다.
- SubType이 invalid면 Type 전체 Row를 보여준다.
- 항목은 `ItemID (DisplayName)` 형식으로 표시된다.

Picker가 비어 있으면 확인할 것:

1. Project Settings에 `_ItemRegistryDataAsset`이 지정되었는지
2. DataAsset의 `_ItemTables`에 DataTable이 등록되었는지
3. `Refresh Registry`를 눌렀는지
4. DataTable Row의 ItemID가 valid인지
5. 현재 Type/SubType 필터가 너무 좁지 않은지

---

## 9. Blueprint에서 ItemID 만들기/분해하기

Blueprint에서 `Make ItemID` 노드를 사용한다.

입력:

| 핀 | 설명 |
| --- | --- |
| Type | ItemType |
| SubType | Type에 대응하는 SubType |
| Serial | 번호 |

출력:

| 핀 | 설명 |
| --- | --- |
| ItemID | 생성된 ItemID |

분해/조회 함수:

- `Break ItemID`
- `Get Type`
- `Get SubType`
- `Get Serial`
- `To Integer`
- `To String`
- `Is Valid`

`Is Valid`는 실패 사유 문자열도 반환한다.

---

## 10. C++에서 Item Row 조회하기

### 10.1 기본 Row 조회

```cpp
FItemTableRow Row;
if (UItemHelper::FindItemRow(ItemID, Row))
{
	// Row 사용
}
```

### 10.2 파생 Row 조회

```cpp
const FMyItemTableRow* Row = UItemHelper::FindItemRow<FMyItemTableRow>(ItemID);
if (IsValid(Row))
{
	// Row->Description 등 사용
}
```

### 10.3 Type별 목록 조회

```cpp
TArray<const FMyItemTableRow*> Rows =
	UItemHelper::GetAllItemRowsByType<FMyItemTableRow>(EItemType::MyItemType, true);
```

두 번째 인자 `_usable_item_only`:

| 값 | 의미 |
| --- | --- |
| `true` | `IsUsableItem()`이 true인 Row만 반환 |
| `false` | Registry에 있는 모든 Row 반환 |

---

## 11. 프로젝트 전용 Helper 만들기

프로젝트에서는 자주 쓰는 타입 전용 Helper를 만드는 것이 좋다.

예:

```cpp
const FMyItemTableRow& UMyItemHelper::GetMyItemRow(FItemID_MyItemType _item_id)
{
	const FMyItemTableRow* item_row_ptr = FindItemRow<FMyItemTableRow>(_item_id);

	if (IsValid(item_row_ptr))
	{
		return *item_row_ptr;
	}

	static const FMyItemTableRow s_null;
	return s_null;
}
```

목록 조회 예:

```cpp
TArray<FMyItemTableRow> UMyItemHelper::GetAllMyItemRows()
{
	const auto item_ptr_rows = GetAllItemRowsByType<FMyItemTableRow>(EItemType::MyItemType);

	TArray<FMyItemTableRow> item_rows;
	item_rows.Reserve(item_ptr_rows.Num());

	for (const FMyItemTableRow* item_ptr : item_ptr_rows)
	{
		if (IsValid(item_ptr))
		{
			item_rows.Add(*item_ptr);
		}
	}

	return item_rows;
}
```

---

## 12. Registry 검증하기

### 12.1 수동 Refresh

툴바의 `Refresh Registry` 버튼을 누른다.

성공 로그:

```text
Register=Success, BuildIndex=Success, RegisteredTables=N, IndexedItems=N
```

실패 로그 예:

| 메시지 | 원인 |
| --- | --- |
| `ItemRegistryDataAsset is not set.` | Project Settings에 Registry DataAsset 미설정 |
| `Failed to load ItemRegistryDataAsset` | 설정된 DataAsset 로드 실패 |
| `Table is null.` | `_ItemTables`에 비어 있는 DataTable entry 있음 |
| `unsupported RowStruct` | `FItemTableRow` 계열이 아닌 DataTable |
| `Invalid ItemID` | Row의 ItemID가 유효하지 않음 |
| `중복 ItemID` | 여러 Row가 같은 ItemID 사용 |

### 12.2 자동 Refresh

- 에디터에서 Engine Subsystem 초기화 시 refresh된다.
- GameInstance 시작 시 `UItemRegistryValidationSubsystem`이 refresh한다.
- DataTable Row 변경 이벤트가 발생하면 `RefreshItemTable()`을 통해 전체 refresh된다.
- `RefreshRegistry()`가 실패해도 실패 전까지 인덱싱된 valid Row가 일부 남을 수 있다. 실패 로그가 있으면 부분 Registry 상태로 간주하고, 조회 테스트를 통과했더라도 원인을 먼저 해결한다.

---

## 13. Debug 표시

비 Shipping 빌드에서 콘솔 명령을 사용할 수 있다.

```text
ToggleShowItemID
```

켜진 상태에서 `FItemTableRow::GetDisplayName()`은 다음처럼 표시된다.

```text
[10010001] 아이템 이름
```

다시 입력하면 꺼진다.

---

## 14. 패키징 전 확인

패키징 전 체크리스트:

1. Project Settings의 `_ItemRegistryDataAsset`가 설정되어 있는지 확인한다.
2. Registry DataAsset의 `_ItemTables`에 필요한 모든 Item DataTable이 들어 있는지 확인한다.
3. `Refresh Registry`가 성공하는지 확인한다.
4. duplicate ItemID가 없는지 확인한다.
5. Shipping에 포함되어야 하는 Row의 `DevState`가 `Shipping`인지 확인한다.
6. 패키징 실행 로그에서 Registry refresh 성공 여부를 확인한다.

주의:

- `_ItemRegistryDataAsset`은 soft reference다.
- 패키징에서 DataAsset/테이블 cook 포함 여부는 프로젝트 설정과 참조 정책에 따라 확인해야 한다.
- 패키징에서 Registry 로드가 실패하면 cook 정책을 별도로 지정해야 할 수 있다.
- 패키징 전 `RefreshRegistry()` 실패가 한 번이라도 있으면 부분 Registry가 남아 있을 수 있으므로, Message Log 오류를 모두 해결한 뒤 다시 refresh 성공 상태를 확인한다.

---

## 15. 추천 작업 순서

새 아이템 타입/테이블을 추가할 때는 이 순서가 안전하다.

1. 프로젝트에 필요한 Type을 `EItemType`에 추가한다.
2. 필요하면 Type 전용 SubType enum을 만든다.
3. `FItemID::GetSubTypeEnum()`에 Type -> SubType enum 매핑을 추가한다.
4. `FItemTableRow`를 상속한 프로젝트 RowStruct를 만든다.
5. DataTable을 만들고 RowStruct를 지정한다.
6. Row마다 ItemID, DisplayName, DevState를 입력한다.
7. `ItemRegistryDataAsset._ItemTables`에 DataTable을 추가한다.
8. Project Settings에 `_ItemRegistryDataAsset`이 지정되어 있는지 확인한다.
9. `Refresh Registry`를 누른다.
10. Message Log 오류를 모두 해결한다.
11. Blueprint picker 또는 C++ Helper로 조회 테스트한다.
12. 필요하면 `ToggleShowItemID`로 UI 표시를 확인한다.

---

## 16. 운영 팁

- ItemID Serial은 1부터 사용한다. 0은 invalid다.
- Type/SubType/Serial 규칙을 팀 내에서 문서화해 duplicate를 줄인다.
- RowStruct 생성자에서 `FItemTableRow(EItemType::...)`를 호출해 Type 실수를 줄인다.
- Registry DataAsset은 source of truth로 취급한다. 임시 DataTable도 Registry에 넣지 않으면 조회되지 않는 것이 정상이다.
- `DevState=NotUsed` Row는 목록 조회에서 기본적으로 제외된다.
- Shipping 테스트 전에는 `DevState`를 반드시 확인한다.
- ItemID picker가 비어 있으면 Registry refresh와 Type/SubType 필터를 먼저 확인한다.
- ItemCore의 Message Log는 `Item Registry` 목록에서 확인한다.
