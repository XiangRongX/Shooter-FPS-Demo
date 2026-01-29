// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShooterAIController.generated.h"

DECLARE_DELEGATE_TwoParams(FShooterPerceptionUpdatedDelegate, AActor*, const FAIStimulus&);
DECLARE_DELEGATE_OneParam(FShooterPerceptionForgottenDelegate, AActor*);

class UStateTreeAIComponent;
class UAIPerceptionComponent;
struct FAIStimulus;

/**
 * 
 */
UCLASS()
class SHOOTER_API AShooterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AShooterAIController();

	void SetCurrentTarget(AActor* InTarget);
	void ClearCurrentTarget();

	FORCEINLINE AActor* GetCurrentTarget() const { return TargetEnemy; }

	FShooterPerceptionUpdatedDelegate OnShooterPerceptionUpdated;
	FShooterPerceptionForgottenDelegate OnShooterPerceptionForgotten;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnPerceptionForgotten(AActor* Actor);

	UFUNCTION()
	void OnPawnDeath();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

    UPROPERTY()
    TObjectPtr<AActor> TargetEnemy;
	
};
