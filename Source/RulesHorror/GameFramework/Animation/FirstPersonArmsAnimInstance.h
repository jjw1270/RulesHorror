// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "FirstPersonArmsAnimInstance.generated.h"

class UAnimSequenceBase;

/*
 * 1인칭 팔 애님 프록시. 워커 스레드에서 Idle/Walk 시퀀스를 샘플링해 _WalkAlpha로 섞음.
 */
struct FFirstPersonArmsAnimInstanceProxy : public FAnimInstanceProxy
{
	FFirstPersonArmsAnimInstanceProxy() = default;
	explicit FFirstPersonArmsAnimInstanceProxy(UAnimInstance* _anim_instance) : FAnimInstanceProxy(_anim_instance) {}

	virtual void PreUpdate(UAnimInstance* _anim_instance, float _delta_seconds) override;
	virtual void Update(float _delta_seconds) override;
	virtual bool Evaluate(FPoseContext& _output) override;

private:
	const UAnimSequenceBase* _IdleAnim = nullptr;
	const UAnimSequenceBase* _WalkAnim = nullptr;
	float _WalkAlpha = 0.0f;
	float _WalkPlayRate = 1.0f;
	float _IdleTime = 0.0f;
	float _WalkTime = 0.0f;
};

/*
 * 1인칭 팔 애님 인스턴스 (애님 블루프린트 없이 사용).
 * 소유 캐릭터(ARulesHorrorCharacter)의 Idle/Walk 시퀀스를 이동 속도에 따라 블렌드함.
 * 애님 블루프린트의 부모 클래스로 써도 된다: 그 블루프린트의 애님 그래프가 포즈를 만들고, 이 클래스는 _WalkAlpha 등 값만 채운다.
 * 팔 교체 방법: Art/Blender/FPArms/README.md "고퀄 팔로 바꾸는 법".
 */
UCLASS(Transient)
class RULESHORROR_API UFirstPersonArmsAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FFirstPersonArmsAnimInstanceProxy;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "First Person")
	TObjectPtr<UAnimSequenceBase> _IdleAnim = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "First Person")
	TObjectPtr<UAnimSequenceBase> _WalkAnim = nullptr;

	/** 0 = Idle, 1 = Walk */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "First Person")
	float _WalkAlpha = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "First Person")
	float _WalkPlayRate = 1.0f;

	/** 이 속도(cm/s)에서 Walk가 100%. 0 이하면 캐릭터 무브먼트의 MaxWalkSpeed 사용 */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	float _FullWalkSpeed = 0.0f;

	/** _WalkAlpha가 목표값으로 따라가는 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	float _BlendInterpSpeed = 8.0f;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float _delta_seconds) override;

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* _proxy) override;
};
