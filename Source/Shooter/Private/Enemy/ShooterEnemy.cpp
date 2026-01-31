// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/ShooterEnemy.h"
#include "Components/CapsuleComponent.h"
#include "ShooterComponent/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Casing.h"
#include "Shooter/Shooter.h"
#include "GameMode/ShooterGameMode.h"
#include "Enemy/ShooterAIController.h"
#include "Character/ShooterPlayerController.h"

AShooterEnemy::AShooterEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponMesh");
	WeaponMesh->SetupAttachment(GetMesh(), FName("RightHandSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(66.f);
	SetMinNetUpdateFrequency(33.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AShooterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	Health = MaxHealth;
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AShooterEnemy::ReceiveDamage);
	}
}

void AShooterEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

void AShooterEnemy::StartFire(AActor* Target)
{
	if (bElimmed || !HasAuthority()) return;

	CurrentHitTarget = Target;
	GetWorldTimerManager().SetTimer(FireTimer, this, &AShooterEnemy::Fire, FireRate, true);
	Fire();
}

void AShooterEnemy::StopFire()
{
	GetWorldTimerManager().ClearTimer(FireTimer);
}

void AShooterEnemy::Fire()
{
	if (!HasAuthority() || bElimmed) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName(TEXT("MuzzleFlash"));
	if (MuzzleFlashSocket && ProjectileClass)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);

		FVector ToTarget = CurrentHitTarget->GetActorLocation() - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;

		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<AActor>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
		}

		Multicast_PlayFireEffects();
	}
}

void AShooterEnemy::Multicast_PlayFireEffects_Implementation()
{
	PlayFireMontage();
}

void AShooterEnemy::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed || !HasAuthority()) return;

	PlayHitReactMontage();
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	if (Health <= 0.f)
	{
		Elim();

		AShooterGameMode* ShooterGameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>();
		if (ShooterGameMode)
		{
			if (ShooterAIController == nullptr)
			{
				ShooterAIController = Cast<AShooterAIController>(Controller);
			}
			AShooterPlayerController* AttackerController = Cast<AShooterPlayerController>(InstigatorController);
			ShooterGameMode->EnemyEliminated(this, ShooterAIController, AttackerController);
			
			UWorld* World = GetWorld();
			if (World)
			{
				World->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterEnemy::BindRespawnTimerFinished, 5.f);
			}
		}
	}
}

void AShooterEnemy::BindRespawnTimerFinished()
{
	AShooterGameMode* ShooterGameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (ShooterGameMode && HasAuthority())
	{
		ShooterGameMode->EnemyRespawn(this, ShooterAIController);
	}
}

void AShooterEnemy::Elim()
{
	bElimmed = true;
	StopFire();
	MulticastElim();

	if (HasAuthority())
	{
		SetLifeSpan(6.0f);
	}
}

void AShooterEnemy::MulticastElim_Implementation()
{
	PlayElimMontage();
	if (GetMesh())
	{
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FVector AShooterEnemy::GetHitTarget() const
{
	//if (IsValid(CurrentHitTarget))
	//{
	//	return CurrentHitTarget->GetActorLocation();
	//}
	return GetActorLocation() + GetActorForwardVector() * 1000.0f;
}

void AShooterEnemy::AimOffset(float DeltaTime)
{
	//if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr) return;

	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir)
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir)
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}


void AShooterEnemy::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}

}

void AShooterEnemy::PlayFireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}

	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(TEXT("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator());
			}
		}
	}
}

void AShooterEnemy::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void AShooterEnemy::PlayReloadMontage()
{

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName = FName("Rifle");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AShooterEnemy::PlayHitReactMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}