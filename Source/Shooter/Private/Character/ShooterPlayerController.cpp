// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "UI/ShooterHUD.h"
#include "UI/CharacterOverlay.h"
#include "UI/Announcement.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameMode/ShooterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Character/ShooterGameState.h"
#include "Character/ShooterPlayerState.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}

	ShooterHUD = Cast<AShooterHUD>(GetHUD());
	ServerCheckMatchState();

	SetInputMode(FInputModeGameOnly());
}

void AShooterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerController, MatchState);
}

void AShooterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	if (ShooterHUD == nullptr)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
	}

	if(ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->HealthBar && ShooterHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		ShooterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		ShooterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AShooterPlayerController::SetHUDScore(float Score)
{
	if (ShooterHUD == nullptr)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
	}

	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		ShooterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void AShooterPlayerController::SetHUDDefeats(int32 Defeats)
{
	if (ShooterHUD == nullptr)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
	}

	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->DefeatAmount)
	{
		FString DefeatText = FString::Printf(TEXT("%d"), Defeats);
		ShooterHUD->CharacterOverlay->DefeatAmount->SetText(FText::FromString(DefeatText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AShooterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	if (ShooterHUD == nullptr)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
	}

	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->WeaponAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		ShooterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AShooterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	if (ShooterHUD == nullptr)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
	}

	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->CarriedAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		ShooterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AShooterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	if (ShooterHUD == nullptr)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
	}

	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->MatchCountdownText)
	{
		if (CountdownInt < 0.f)
		{
			ShooterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ShooterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountText));
	}
}

void AShooterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	if (ShooterHUD == nullptr)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
	}

	if (ShooterHUD && ShooterHUD->Announcement && ShooterHUD->Announcement->WarmupTime)
	{
		if (CountdownInt < 0.f)
		{
			ShooterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ShooterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountText));
	}
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn);
	if (ShooterCharacter)
	{
		SetHUDHealth(ShooterCharacter->GetHealth(), ShooterCharacter->GetMaxHealth());
	}
}

void AShooterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

float AShooterPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}
}

void AShooterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AShooterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (HasAuthority())
	{
		if (ShooterGameMode == nullptr)
		{
			ShooterGameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
		}
		if (ShooterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(ShooterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void AShooterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (ShooterHUD && ShooterHUD->CharacterOverlay)
		{
			CharacterOverlay = ShooterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

void AShooterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AShooterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void AShooterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AShooterPlayerController::ServerCheckMatchState_Implementation()
{
	AShooterGameMode* GameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();

		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		/*if (ShooterHUD && MatchState==MatchState::WaitingToStart)
		{
			ShooterHUD->AddAnnouncement();
		}*/
	}
}

void AShooterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;

	OnMatchStateSet(MatchState);

	if (ShooterHUD && MatchState == MatchState::WaitingToStart)
	{
		ShooterHUD->AddAnnouncement();
	}
}

void AShooterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AShooterPlayerController::HandleMatchHasStarted()
{
	if (MatchState == MatchState::InProgress)
	{
		if (ShooterHUD == nullptr)
		{
			ShooterHUD = Cast<AShooterHUD>(GetHUD());
		}
		if (ShooterHUD)
		{
			ShooterHUD->AddCharacterOverlay();
			if (ShooterHUD->Announcement)
			{
				ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void AShooterPlayerController::HandleCooldown()
{
	if (MatchState == MatchState::Cooldown)
	{
		if (ShooterHUD == nullptr)
		{
			ShooterHUD = Cast<AShooterHUD>(GetHUD());
		}
		if (ShooterHUD)
		{
			ShooterHUD->CharacterOverlay->RemoveFromParent();
			if (ShooterHUD->Announcement && ShooterHUD->Announcement->AnnouncementText && ShooterHUD->Announcement->AnnouncementInfo)
			{
				ShooterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
				ShooterHUD->Announcement->AnnouncementText->SetText(FText::FromString(TEXT("距离下场比赛")));

				AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
				AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
				if (ShooterGameState && ShooterPlayerState)
				{
					TArray<AShooterPlayerState*> TopPlayers = ShooterGameState->TopScoringPlayers;
					FString InfoText;
					if (TopPlayers.Num() == 0)
					{
						InfoText = FString(TEXT("无人胜出:("));
					}
					else if (TopPlayers.Num() == 1 && TopPlayers[0] == ShooterPlayerState)
					{
						InfoText = FString(TEXT("你是获胜者！"));
					}
					else if (TopPlayers.Num() == 1)
					{
						InfoText = FString::Printf(TEXT("获胜者：\n%s"), *TopPlayers[0]->GetPlayerName());
					}
					else if (TopPlayers.Num() > 1)
					{
						InfoText = FString(TEXT("获胜者：\n"));
						for (auto TiedPlayer : TopPlayers)
						{
							InfoText.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
						}
					}
					ShooterHUD->Announcement->AnnouncementInfo->SetText(FText::FromString(InfoText));
				}
			}
		}

		SetInputMode(FInputModeUIOnly());
		FlushPressedKeys();
	}
}
