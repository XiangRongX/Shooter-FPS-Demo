// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyAnimInstance.h"
#include "Enemy/ShooterEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"
#include "ShooterTypes/CombatState.h"

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ShooterEnemy = Cast<AShooterEnemy>(TryGetPawnOwner());
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (ShooterEnemy == nullptr)
	{
		ShooterEnemy = Cast<AShooterEnemy>(TryGetPawnOwner());
	}
	if (ShooterEnemy == nullptr) return;

	FVector Velocity = ShooterEnemy->GetVelocity();
	Velocity.Z = 0;
	Speed = Velocity.Size();

	bIsInAir = ShooterEnemy->GetCharacterMovement()->IsFalling();
	bIsAccelerating = ShooterEnemy->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f || ShooterEnemy->GetVelocity().Size() > 10.f;;
	bAiming = ShooterEnemy->IsAiming();
	TurningInPlace = ShooterEnemy->GetTurningInPlace();
	bElimmed = ShooterEnemy->IsElimmed();

	FRotator AimRotation = ShooterEnemy->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterEnemy->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterEnemy->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean ? Lean : 0.f, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = ShooterEnemy->GetAO_Yaw();
	AO_Pitch = ShooterEnemy->GetAO_Pitch();

	if (bWeaponEquipped && ShooterEnemy->GetMesh())
	{
		LeftHandTransform = ShooterEnemy->WeaponMesh->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		ShooterEnemy->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (ShooterEnemy->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = ShooterEnemy->WeaponMesh->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - ShooterEnemy->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 30.f);
		}
	}

}
