// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterEnemy.generated.h"

UCLASS()
class SHOOTER_API AShooterEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AShooterEnemy();
	virtual void Tick(float DeltaTime) override;

	void StartFire(const FVector& Target); 
	void StopFire();
	void Elim();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffects();

	FORCEINLINE bool IsElimmed() const { return bElimmed; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

private:
	UPROPERTY(EditAnywhere, Category = "AI Stats")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, Category = "AI Stats")
	float MaxHealth = 100.f;

	bool bElimmed = false;

	FTimerHandle FireTimer;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	float FireRate = 0.15f;

	UPROPERTY(EditAnywhere, Category = "AI Combat")
	TSubclassOf<AActor> ProjectileClass; 

	FVector CurrentHitTarget;
	void Fire();

};
