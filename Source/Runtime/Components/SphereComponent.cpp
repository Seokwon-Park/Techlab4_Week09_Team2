#include "EnginePCH.h"
#include "SphereComponent.h"

float USphereComponent::GetScaledSphereRadius() const
{
	// TODO(D): 월드 스케일 최대 축 반영
	return SphereRadius;
}

FBox USphereComponent::CalcLocalBounds() const
{
	const FVector R(SphereRadius, SphereRadius, SphereRadius);
	return FBox{ R * -1.0f, R };
}
