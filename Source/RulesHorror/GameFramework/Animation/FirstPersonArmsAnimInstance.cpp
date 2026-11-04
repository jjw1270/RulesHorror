// Copyright (c) 2026 장윤제. All rights reserved.

#include "GameFramework/Animation/FirstPersonArmsAnimInstance.h"
#include "GameFramework/Pawn/RulesHorrorCharacter.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "CommonUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	void ExtractPose(const UAnimSequenceBase* _anim, float _time, FPoseContext& _output)
	{
		FAnimationPoseData pose_data(_output);
		_anim->GetAnimationPose(pose_data, FAnimExtractContext(static_cast<double>(_time), false, {}, true));
	}

	float AdvanceLoopTime(const UAnimSequenceBase* _anim, float _time, float _delta_seconds)
	{
		if (_anim == nullptr)
		{
			return 0.0f;
		}

		const float length = _anim->GetPlayLength();
		if (length <= KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}

		return FMath::Fmod(_time + _delta_seconds, length);
	}
}

void FFirstPersonArmsAnimInstanceProxy::PreUpdate(UAnimInstance* _anim_instance, float _delta_seconds)
{
	FAnimInstanceProxy::PreUpdate(_anim_instance, _delta_seconds);

	// 게임 스레드에서 인스턴스 값을 복사
	const UFirstPersonArmsAnimInstance* arms_instance = CastChecked<UFirstPersonArmsAnimInstance>(_anim_instance);
	_IdleAnim = arms_instance->_IdleAnim;
	_WalkAnim = arms_instance->_WalkAnim;
	_WalkAlpha = arms_instance->_WalkAlpha;
	_WalkPlayRate = arms_instance->_WalkPlayRate;
}

void FFirstPersonArmsAnimInstanceProxy::Update(float _delta_seconds)
{
	_IdleTime = AdvanceLoopTime(_IdleAnim, _IdleTime, _delta_seconds);

	// 멈춰 있을 땐 다음 걸음을 처음부터 시작하도록 되감음
	_WalkTime = (_WalkAlpha > KINDA_SMALL_NUMBER) ? AdvanceLoopTime(_WalkAnim, _WalkTime, _delta_seconds * _WalkPlayRate) : 0.0f;
}

bool FFirstPersonArmsAnimInstanceProxy::Evaluate(FPoseContext& _output)
{
	if (_IdleAnim == nullptr && _WalkAnim == nullptr)
	{
		_output.ResetToRefPose();
		return true;
	}

	const float walk_alpha = (_IdleAnim == nullptr) ? 1.0f : ((_WalkAnim == nullptr) ? 0.0f : _WalkAlpha);
	if (walk_alpha <= KINDA_SMALL_NUMBER)
	{
		ExtractPose(_IdleAnim, _IdleTime, _output);
		return true;
	}

	if (walk_alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		ExtractPose(_WalkAnim, _WalkTime, _output);
		return true;
	}

	FPoseContext idle_pose(_output);
	FPoseContext walk_pose(_output);
	ExtractPose(_IdleAnim, _IdleTime, idle_pose);
	ExtractPose(_WalkAnim, _WalkTime, walk_pose);

	FAnimationPoseData output_pose_data(_output);
	FAnimationRuntime::BlendTwoPosesTogether(FAnimationPoseData(idle_pose), FAnimationPoseData(walk_pose), 1.0f - walk_alpha, output_pose_data);
	return true;
}

void UFirstPersonArmsAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	auto character = Cast<ARulesHorrorCharacter>(GetOwningActor());
	if (IsValid(character))
	{
		_IdleAnim = character->GetFirstPersonIdleAnim();
		_WalkAnim = character->GetFirstPersonWalkAnim();
	}

	_WalkAlpha = 0.0f;
	_WalkPlayRate = 1.0f;
}

void UFirstPersonArmsAnimInstance::NativeUpdateAnimation(float _delta_seconds)
{
	Super::NativeUpdateAnimation(_delta_seconds);

	float target_alpha = 0.0f;
	float play_rate = 1.0f;

	auto character = Cast<ACharacter>(GetOwningActor());
	if (IsValid(character))
	{
		const UCharacterMovementComponent* movement = character->GetCharacterMovement();
		const float full_walk_speed = (_FullWalkSpeed > 0.0f) ? _FullWalkSpeed : (IsValid(movement) ? movement->MaxWalkSpeed : 250.0f);
		const bool is_on_ground = IsValid(movement) ? movement->IsMovingOnGround() : true;
		const float speed = character->GetVelocity().Size2D();

		if (is_on_ground && full_walk_speed > KINDA_SMALL_NUMBER)
		{
			const float speed_ratio = speed / full_walk_speed;
			target_alpha = FMath::Clamp(speed_ratio, 0.0f, 1.0f);
			play_rate = FMath::Clamp(speed_ratio, 0.5f, 1.5f);
		}
	}

	_WalkAlpha = FMath::FInterpTo(_WalkAlpha, target_alpha, _delta_seconds, _BlendInterpSpeed);
	if (_WalkAlpha < 0.01f && target_alpha <= 0.0f)
	{
		_WalkAlpha = 0.0f;
	}
	_WalkPlayRate = play_rate;
}

FAnimInstanceProxy* UFirstPersonArmsAnimInstance::CreateAnimInstanceProxy()
{
	return new FFirstPersonArmsAnimInstanceProxy(this);
}

void UFirstPersonArmsAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* _proxy)
{
	delete _proxy;
}
