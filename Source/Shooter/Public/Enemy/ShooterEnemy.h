// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterTypes/TurningInPlace.h"
#include "Interface/InteractWithCrosshairInterface.h"
#include "ShooterEnemy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPawnDeathDelegate);

class ACasing;
class AShooterAIController;

UCLASS()
class SHOOTER_API AShooterEnemy : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	AShooterEnemy();
	virtual void Tick(float DeltaTime) override;

	void StartFire(AActor* Target);
	void StopFire();
	void Elim();
	void AimOffset(float DeltaTime);
	void TurnInPlace(float DeltaTime);
	void PlayFireMontage();
	void PlayElimMontage();
	void PlayReloadMontage();
	void PlayHitReactMontage();
	FVector GetHitTarget() const;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffects();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	FPawnDeathDelegate OnPawnDeath;

	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE bool IsAiming() const { return bAiming; }
	FORCEINLINE void SetAiming(bool Aiming) { bAiming = Aiming; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

private:
	UPROPERTY()
	TObjectPtr<AShooterAIController> ShooterAIController;

	UPROPERTY(EditAnywhere, Category = "AI Stats", meta = (AllowPrivateAccess = "true"))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, Category = "AI Stats")
	float MaxHealth = 100.f;

	bool bElimmed = false;
	bool bAiming = false;

	FTimerHandle FireTimer;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	float FireRate = 0.5f;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	TSubclassOf<AActor> ProjectileClass; 

	AActor* CurrentHitTarget;
	void Fire();

	ETurningInPlace TurningInPlace;
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	TObjectPtr<UAnimMontage> ElimMontage;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACasing> CasingClass;
};
