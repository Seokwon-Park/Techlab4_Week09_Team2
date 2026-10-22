#include "EnginePCH.h"
#include "BoxComponent.h"

FVector UBoxComponent::GetScaledBoxExtent() const
{
	// TODO(D): 월드 스케일 반영
	return BoxExtent;
}

FBox UBoxComponent::CalcLocalBounds() const
{
	return FBox{ BoxExtent * -1.0f, BoxExtent };
}
