// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StoryFlowDefines.h"
#include "RulesHorrorProgressDefines.generated.h"

/*
 * 전역 진행 플래그. 값 이름이 그대로 SaveGame 키가 된다(예: "Brand_01_Oppression").
 * 이름을 바꾸면 기존 저장의 값을 읽지 못하므로 바꾸지 말고 새 값을 추가할 것.
 * 기준 문서: 기획/챕터-씬-기획서.md "3. 전역 진행 상태 / Save Key".
 */
UENUM(BlueprintType)
enum class ERulesHorrorProgressFlag : uint8
{
	None UMETA(Hidden),

	Scenario_01_Unlocked	UMETA(ToolTip = "사이트 첫 번째 칸(PC방 시나리오) 진입 가능. CH02_S03"),
	Scenario_02_Unlocked	UMETA(ToolTip = "사이트 두 번째 칸(무궁화 시나리오) 진입 가능. CH03_S01"),
	Scenario_03_Unlocked	UMETA(ToolTip = "사이트 세 번째 칸(숲 탈출 시나리오) 진입 가능. CH03_S05"),

	Scenario_01_Clear		UMETA(ToolTip = "PC방 시나리오 클리어. CH02_S04 완료"),
	Scenario_02_Clear		UMETA(ToolTip = "무궁화 시나리오 클리어. CH03_S03 완료"),
	Scenario_03_Clear		UMETA(ToolTip = "숲 탈출 시나리오 클리어. CH04_S04 완료"),

	Brand_01_Oppression		UMETA(ToolTip = "첫 번째 낙인(억압) 획득. CH02_S05"),
	Brand_02_Bystanding		UMETA(ToolTip = "두 번째 낙인(방관) 획득. CH03_S04"),
	Brand_03_Exploitation	UMETA(ToolTip = "세 번째 낙인(착취) 획득. CH04_S05"),

	EndingUnlocked			UMETA(ToolTip = "병실 엔딩 진입 가능(마지막 소원 입력 완료). CH05_S04"),

	Site_SignedUp			UMETA(ToolTip = "괴담 사이트 i-wish 가입(닉네임) 완료. CH02_S03. 켜져 있으면 모니터가 부팅·가입 화면을 건너뛰고 바탕화면부터 보여 준다."),

	Max UMETA(Hidden),
};

/* 사이트의 세 시나리오. 시나리오 N을 클리어하면 낙인 N을 얻는다. */
UENUM(BlueprintType)
enum class ERulesHorrorScenario : uint8
{
	None,
	Scenario_01_PCBang		UMETA(ToolTip = "규칙괴담 PC방 - 낙인1 억압"),
	Scenario_02_Mugunghwa	UMETA(ToolTip = "무궁화 꽃이 피었습니다 - 낙인2 방관"),
	Scenario_03_Forest		UMETA(ToolTip = "칠흙 숲 탈출 - 낙인3 착취"),
};

namespace RulesHorrorProgress
{
	constexpr int32 ScenarioCount = 3;

	RULESHORROR_API bool IsValidFlag(ERulesHorrorProgressFlag _flag);
	RULESHORROR_API FName ToSaveKey(ERulesHorrorProgressFlag _flag);

	RULESHORROR_API ERulesHorrorProgressFlag GetUnlockedFlag(ERulesHorrorScenario _scenario);
	RULESHORROR_API ERulesHorrorProgressFlag GetClearFlag(ERulesHorrorScenario _scenario);
	RULESHORROR_API ERulesHorrorProgressFlag GetBrandFlag(ERulesHorrorScenario _scenario);

	/* "CH02_S01" -> 2. 챕터 형식(CH##_)이 아니면 INDEX_NONE. */
	RULESHORROR_API int32 ParseChapterNumber(const FStorySceneID& _scene_id);
}
