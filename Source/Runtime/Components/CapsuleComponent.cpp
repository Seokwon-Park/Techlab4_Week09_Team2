#include "EnginePCH.h"
#include "CapsuleComponent.h"

float UCapsuleComponent::GetScaledCapsuleHalfHeight() const
{
	// TODO(D): 월드 Z 스케일 반영
	return CapsuleHalfHeight;
}

float UCapsuleComponent::GetScaledCapsuleRadius() const
{
	// TODO(D): 월드 X·Y 스케일 중 큰 값 반영
	return CapsuleRadius;
}

FBox UCapsuleComponent::CalcLocalBounds() const
{
	const FVector E(CapsuleRadius, CapsuleRadius, CapsuleHalfHeight);
	return FBox{ E * -1.0f, E };
}
