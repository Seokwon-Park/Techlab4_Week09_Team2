#include "EnginePCH.h"
#include "SpringArmComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

USpringArmComponent::USpringArmComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::PostPhysics; // 폰 이동 후
}

void USpringArmComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	UpdateDesiredArmLocation(
		bDoCollisionTest,
		bEnableCameraLag,
		bEnableCameraRotationLag,
		DeltaTime);
}

void USpringArmComponent::UpdateDesiredArmLocation(
	bool bDoTrace,
	bool bDoLocationLag,
	bool bDoRotationLag,
	float DeltaTime)
{
	// 1) 회전 (+ Rotation Lag). UE는 쿼터니언 QInterpTo (최단 경로, 짐벌락 없음)
	FRotator DesiredRot = GetTargetRotation();
	if (bDoRotationLag)
	{
		// TODO: QInterpTo 구현 필요, 구현 후 아래 주석 해제
		//DesiredRot = FRotator(
		//	FMath::QInterpTo(
		//		PreviousDesiredRot.Quaternion(),
		//		DesiredRot.Quaternion(),
		// 		DeltaTime,
		//		CameraRotationLagSpeed));
	}
	PreviousDesiredRot = DesiredRot;

	// 2) 팔 시작점 (+ Location Lag는 "시작점"에 건다: 팔 길이는 유지된 채 늦게 따라옴
	const FVector ArmOrigin = GetWorldLocation() + TargetOffset;
	FVector DesiredLoc = ArmOrigin;
	if (bDoLocationLag)
	{
		// TODO: VInterpTo 구현 필요, 구현 후 아래 주석 해제
		//DesiredLoc = FMath::VInterpTo(
		//	PreviousDesiredLoc,
		//	ArmOrigin,
		//	DeltaTime,
		//	CameraLagSpeed);
		const FVector FromOrigin = DesiredLoc - ArmOrigin;
		if (CameraLagMaxDistance > 0.0f &&
			FromOrigin.Size() > CameraLagMaxDistance)
		{
			DesiredLoc = ArmOrigin
				+ FromOrigin.GetSafeNormal() * CameraLagMaxDistance;
		}
	}
	PreviousDesiredLoc = DesiredLoc;

	// 3) 팔을 뒤로 뻗고 소켓 오프셋 적용
	const FQuat Q = DesiredRot.Quaternion();
	DesiredLoc -= Q.GetForwardVector() * TargetArmLength;
	DesiredLoc += Q.RotateVector(SocketOffset);

	// 4) 충돌: 시작점 -> 원하는 위치로 구를 쓸어서, 막히면 맞은 거리에서 멈춘다
	FVector ResultLoc = DesiredLoc;
	if (bDoTrace && TargetArmLength > 0.0f)
	{
		const FVector ToCamera = DesiredLoc - ArmOrigin;
		const float Len = ToCamera.Size();
		FHitResult Hit;
		if (Len > 1.e-4f &&
			GetWorld()->SweepSingle(FRay(ArmOrigin, ToCamera / Len), Len, ProbeSize, Hit, GetOwner(), true))
		{
			ResultLoc = Hit.Location;
		}
	}

	// 5) 자식 카메라에 반영.
	for (USceneComponent* Child : GetAttachChildren())
	{
		if (Child)
		{
			Child->SetWorldLocationAndRotation(ResultLoc, DesiredRot, false);
		}
	}
}

void USpringArmComponent::OnRegister()
{
	Super::OnRegister();
	UpdateDesiredArmLocation(false, false, false, 0.0f);
}

FRotator USpringArmComponent::GetTargetRotation() const
{
	FRotator R = GetWorldRotation();
	if (bUsePawnControlRotation)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			if (APlayerController* PC = Pawn->GetController())
			{
				// TODO: PlayerController.ControlRotation 구현 필요, 구현 후 아래 주석 해제
				//R = PC->ControlRotation;
			}
		}
	}

	const FRotator Rel = GetRelativeRotation();
	if (!bInheritPitch)
	{
		R.Pitch = Rel.Pitch;
	}
	if (!bInheritYaw)
	{
		R.Yaw = Rel.Yaw;
	}
	if (!bInheritRoll)
	{
		R.Roll = Rel.Roll;
	}

	return R;
}