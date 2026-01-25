// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/ShooterEnemy.h"
#include "Components/CapsuleComponent.h"
#include "ShooterComponent/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"

AShooterEnemy::AShooterEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponMesh");
	WeaponMesh->SetupAttachment(GetMesh(), FName("RightHandSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(66.f);
	SetMinNetUpdateFrequency(33.f);
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

}

void AShooterEnemy::StartFire(const FVector& Target)
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

		FVector ToTarget = CurrentHitTarget - SocketTransform.GetLocation();
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
	
}

void AShooterEnemy::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed || !HasAuthority()) return;

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	if (Health <= 0.f)
	{
		Elim();
	}
}

void AShooterEnemy::Elim()
{
	bElimmed = true;
	StopFire();
	MulticastElim();
}

void AShooterEnemy::MulticastElim_Implementation()
{
	if (GetMesh())
	{
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}