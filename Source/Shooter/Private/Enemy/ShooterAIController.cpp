// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/ShooterAIController.h"
#include "Enemy/ShooterEnemy.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AI/Navigation/PathFollowingAgentInterface.h"

AShooterAIController::AShooterAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AShooterAIController::OnPerceptionUpdated);
	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &AShooterAIController::OnPerceptionForgotten);
}

void AShooterAIController::SetCurrentTarget(AActor* InTarget)
{
	TargetEnemy = InTarget;
}

void AShooterAIController::ClearCurrentTarget()
{
	TargetEnemy = nullptr;
}

void AShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AShooterEnemy* Enemy = Cast<AShooterEnemy>(InPawn))
	{
		Enemy->OnPawnDeath.AddDynamic(this, &AShooterAIController::OnPawnDeath);
	}

	if (StateTreeComponent)
	{
		StateTreeComponent->StopLogic("Dead");
		StateTreeComponent->StartLogic();
	}
}

void AShooterAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	OnShooterPerceptionUpdated.ExecuteIfBound(Actor, Stimulus);
}

void AShooterAIController::OnPerceptionForgotten(AActor* Actor)
{
	OnShooterPerceptionForgotten.ExecuteIfBound(Actor);
}

void AShooterAIController::OnPawnDeath()
{
	GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::UserAbort);
	StateTreeComponent->StopLogic(FString(""));
	UnPossess();
	Destroy();
}
