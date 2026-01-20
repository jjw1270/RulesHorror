// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RulesHorror.h"
#include "RulesHorrorCharacter.generated.h"

class UInputAction;
struct FInputActionValue;

/*
*
 */
UCLASS(abstract)
class ARulesHorrorCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UCameraComponent* FirstPersonCameraComponent;

	/** Player light source */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USpotLightComponent* SpotLight;

protected:
	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* LookAction;

protected:
	UPROPERTY(EditAnywhere, Category = "Walk")
	float WalkSpeed = 250.0f;

public:
	ARulesHorrorCharacter();

protected:
	void MoveInput(const FInputActionValue& Value);
	void LookInput(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

public:
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};
