// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ShooterGameMode.generated.h"

class AShooterCharacter;
class AShooterEnemy;
class AShooterPlayerController;
class AShooterAIController;
class AEnemyStart;
class APlayerStart;

namespace MatchState
{
	extern SHOOTER_API const FName Cooldown;
}

/**
 * 
 */
UCLASS()
class SHOOTER_API AShooterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AShooterGameMode();
	virtual void Tick(float DeltaTime) override;
	AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void PlayerEliminated(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController);
	virtual void PlayerEliminatedByEnemy(AShooterCharacter* ElimmedCharacter, AShooterPlayerController* VictimController, AShooterAIController* AttackerController);
	virtual void EnemyEliminated(AShooterEnemy* ElimmedCharacter, AShooterAIController* VictimController, AShooterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	virtual void EnemyRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	void SpawnEnemies();

	FORCEINLINE float GetCountdownTime() { return CountdownTime; }

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

	UPROPERTY(EditDefaultsOnly)
	int32 NumEnemies = 3;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AShooterEnemy> EnemyClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<APlayerStart> PlayerStartClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AEnemyStart> EnemyStartClass;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;

};
