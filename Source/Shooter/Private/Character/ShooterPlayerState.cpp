// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ShooterPlayerState.h"
#include "Character/ShooterCharacter.h"
#include "Character/ShooterPlayerController.h"
#include "Net/UnrealNetwork.h"

void AShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerState, Defeats);
}

void AShooterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	if (Character == nullptr)
	{
		Character = Cast<AShooterCharacter>(GetPawn());
	}
	if (Character)
	{
		if (Controller == nullptr)
		{
			Controller = Cast<AShooterPlayerController>(Character->Controller);
		}
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void AShooterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);

	if (Character == nullptr)
	{
		Character = Cast<AShooterCharacter>(GetPawn());
	}
	if (Character)
	{
		if (Controller == nullptr)
		{
			Controller = Cast<AShooterPlayerController>(Character->Controller);
		}
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void AShooterPlayerState::OnRep_Defeats()
{
	if (Character == nullptr)
	{
		Character = Cast<AShooterCharacter>(GetPawn());
	}
	if (Character)
	{
		if (Controller == nullptr)
		{
			Controller = Cast<AShooterPlayerController>(Character->Controller);
		}
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void AShooterPlayerState::AddToDefeats(int32 DefeatAmount)
{
	Defeats += DefeatAmount;
	if (Character == nullptr)
	{
		Character = Cast<AShooterCharacter>(GetPawn());
	}
	if (Character)
	{
		if (Controller == nullptr)
		{
			Controller = Cast<AShooterPlayerController>(Character->Controller);
		}
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}
