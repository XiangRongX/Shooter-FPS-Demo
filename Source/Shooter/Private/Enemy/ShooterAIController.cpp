// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/ShooterAIController.h"
#include "Enemy/ShooterEnemy.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Navigation/PathFollowingComponent.h"

AShooterAIController::AShooterAIController()
{
    // 初始化 StateTree
    StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));

    // 配置感知系统
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    if (SightConfig)
    {
        SightConfig->SightRadius = 3000.f;
        SightConfig->LoseSightRadius = 3500.f;
        SightConfig->PeripheralVisionAngleDegrees = 90.f;
        // 确保能检测到所有阵营，后续通过 Tag 过滤
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

        AIPerceptionComponent->ConfigureSense(*SightConfig);
        AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    }

    // 绑定感知委托
    AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AShooterAIController::OnPerceptionUpdated);
}

void AShooterAIController::SetCurrentTarget(AActor* InTarget)
{
    CurrentTarget = InTarget;

    // 逻辑驱动：如果 StateTree 正在运行，它会自动从控制器读取 CurrentTarget
    // 如果没有 StateTree，我们可以直接驱动 Pawn 开火
    if (AShooterEnemy* Enemy = Cast<AShooterEnemy>(GetPawn()))
    {
        if (CurrentTarget)
        {
            Enemy->StartFire(CurrentTarget->GetActorLocation());
        }
        else
        {
            Enemy->StopFire();
        }
    }
}

void AShooterAIController::BeginPlay()
{
    Super::BeginPlay();


}

void AShooterAIController::OnPossess(APawn* InPawn)
{

    Super::OnPossess(InPawn);
}

void AShooterAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    // 仅针对带有 Player 标签的 Actor 产生反应
    if (Actor->ActorHasTag(EnemyTag))
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            SetCurrentTarget(Actor);
        }
        else
        {
            // 如果丢失视野，延迟一段时间或立即清除目标
            ClearCurrentTarget();
        }
    }
}

void AShooterAIController::OnControlledPawnDeath()
{
    if (GetPathFollowingComponent())
    {
        GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::UserAbort);
    }

    StateTreeComponent->StopLogic(FString("Pawn Died"));
    UnPossess();
    Destroy(); // 销毁控制器释放服务器内存
}
