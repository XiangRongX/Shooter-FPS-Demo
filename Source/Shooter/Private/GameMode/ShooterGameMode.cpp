// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/ShooterGameMode.h"
#include "Character/ShooterCharacter.h"
#include "Character/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Character/ShooterPlayerState.h"
#include "Character/ShooterGameState.h"
#include "Enemy/EnemyStart.h"
#include "Enemy/ShooterEnemy.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AShooterGameMode::AShooterGameMode()
{
	bDelayedStart = true;
	bUseSeamlessTravel = true;
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AShooterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
			SpawnEnemies();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = WarmupTime + MatchTime + CooldownTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

AActor* AShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	APawn* PawnToSpawn = Player->GetPawn();

	FName TargetTag = Player->IsPlayerController() ? FName("PlayerStart") : FName("EnemyStart");

	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), TargetTag, FoundStarts);

	if (FoundStarts.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, FoundStarts.Num() - 1);
		return FoundStarts[RandomIndex];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AShooterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		AShooterPlayerController* ShooterPlayer = Cast<AShooterPlayerController>(*It);
		if (ShooterPlayer)
		{
			ShooterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void AShooterGameMode::PlayerEliminated(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;

	AShooterPlayerState* AttackerPlayerState = AttackerController ? Cast<AShooterPlayerState>(AttackerController->PlayerState) : nullptr;
	AShooterPlayerState* VictimPlayerState = VictimController ? Cast<AShooterPlayerState>(VictimController->PlayerState) : nullptr;
	AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && ShooterGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		ShooterGameState->UpdateTopScore(AttackerPlayerState);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if(ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void AShooterGameMode::PlayerEliminatedByEnemy(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterAIController* AttackerController)
{
	if (AttackerController == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;

	AShooterPlayerState* VictimPlayerState = VictimController ? Cast<AShooterPlayerState>(VictimController->PlayerState) : nullptr;

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void AShooterGameMode::EnemyEliminated(AShooterEnemy* ElimmedCharacter, AShooterAIController* VictimController, AShooterPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr) return;

	AShooterPlayerState* AttackerPlayerState = AttackerController ? Cast<AShooterPlayerState>(AttackerController->PlayerState) : nullptr;
	AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
	if (AttackerPlayerState && ShooterGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		ShooterGameState->UpdateTopScore(AttackerPlayerState);
	}
}

void AShooterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if(ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if(ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClassWithTag(this, PlayerStartClass, FName("PlayerStart"), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void AShooterGameMode::EnemyRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedController)
	{
		TArray<AActor*> EnemyStarts;
		UGameplayStatics::GetAllActorsOfClass(this, EnemyStartClass, EnemyStarts);
		int32 Selection = FMath::RandRange(0, EnemyStarts.Num() - 1);
		AActor* SpawnPoint = EnemyStarts[Selection];
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = GetInstigator();
		AShooterEnemy* NewEnemy = GetWorld()->SpawnActor<AShooterEnemy>(
			EnemyClass,
			SpawnPoint->GetActorLocation(),
			SpawnPoint->GetActorRotation(),
			SpawnParams
		);
		if (NewEnemy)
		{
			ElimmedController->Possess(NewEnemy);
		}
	}
}

void AShooterGameMode::SpawnEnemies()
{
	TArray<AActor*> EnemyStarts;
	UGameplayStatics::GetAllActorsOfClass(this, AEnemyStart::StaticClass(), EnemyStarts);
	for (int i = 0; i < NumEnemies; i++)
	{
		int32 Selection = FMath::RandRange(0, EnemyStarts.Num() - 1);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* SelectedSpawnPoint = EnemyStarts[Selection];
		FVector SpawnLocation = SelectedSpawnPoint->GetActorLocation();
		FRotator SpawnRotation = SelectedSpawnPoint->GetActorRotation();
		AShooterEnemy* NewEnemy = GetWorld()->SpawnActor<AShooterEnemy>(EnemyClass, SpawnLocation, SpawnRotation, SpawnParams);
		if (NewEnemy)
		{
			NewEnemy->SpawnDefaultController();
		}
	}
	
}


