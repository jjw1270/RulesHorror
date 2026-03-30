// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Computer.generated.h"

UCLASS()
class RULESHORROR_API AComputer : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class USceneComponent> Root = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UStaticMeshComponent> MonitorMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<class UWidgetComponent> ScreenWidgetComponent = nullptr;
	
protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UMaterialInstance> _ScreenMaterialInstance = nullptr;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UMaterialInstanceDynamic> _DynamicScreenMaterialInstance = nullptr;

	const FName TextureParamName = TEXT("Display Input");
	const FName FilterParamName = TEXT("Filter On");



public:	
	AComputer();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, meta = (Tooltip = "전원 켜기/끄기"))
	void SetPower(bool _on);

protected:
	void UpdateScreenFromWidget();

};
