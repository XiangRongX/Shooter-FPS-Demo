// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ShooterPlayerController.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(Subsystem);

		if (NumberOfPlayers == Subsystem->DesiredNumPublicConnections)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true;
				World->GetTimerManager().SetTimer(JumpTimerHandle, 
					[World]()
					{
						World->ServerTravel(FString("/Game/Maps/ShooterMap?listen"));
					}, 
					10.f, false);

				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(It->Get());
					if (PlayerController)
					{
						PlayerController->ClientShowLobbyTips(true);
					}
				}
			}
		}
	}
}

void ALobbyGameMode::Logout(AController* Existing)
{
	Super::Logout(Existing);

	if (GameState)
	{
		int32 NumberOfPlayers = GameState->PlayerArray.Num();

		if (NumberOfPlayers < RequiredPlayers)
		{
			if (GetWorldTimerManager().IsTimerActive(JumpTimerHandle))
			{
				GetWorldTimerManager().ClearTimer(JumpTimerHandle);

				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(It->Get());
					if (PlayerController)
					{
						PlayerController->ClientShowLobbyTips(false);
					}
				}
			}
		}
	}
}
