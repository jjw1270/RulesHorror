// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RulesHorrorStoryDefines.generated.h"

/* 대사/자막 한 줄. Speaker가 비면 지문(괄호 설명)으로 보여준다. Text 안의 {Nickname}은 저장된 닉네임으로 바뀐다. */
USTRUCT(BlueprintType)
struct RULESHORROR_API FStoryDialogueLine
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FText Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (MultiLine = true))
	FText Text;

	/* 이 시간이 지나면 다음 줄로 넘어간다. 0이면 글자 수로 정한다. 클릭/스페이스/엔터로 바로 넘길 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (ClampMin = "0.0", Units = "s"))
	float Duration = 0.0f;
};

/* Shot_WaitStoryEvent가 기다리는 이벤트 이름. 조사 오브젝트, 트리거, 컴퓨터, 사이트 UI가 보낸다. */
namespace RulesHorrorStoryEvents
{
	/* 컴퓨터 앞으로 이동이 끝나 모니터 화면을 쓸 수 있게 됨 */
	inline const FName ComputerOpened(TEXT("Computer_Opened"));
	/* 사이트 가입(닉네임 화면)을 마침 */
	inline const FName SiteSignedUp(TEXT("Site_SignedUp"));
	/* 사이트에서 괴담(시나리오) 입장을 누름 */
	inline const FName SiteStoryPlay(TEXT("Site_StoryPlay"));
}
