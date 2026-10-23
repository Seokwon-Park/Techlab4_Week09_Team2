#pragma once

#include "ShapeComponent.h"

class USphereComponent : public UShapeComponent
{
	DECLARE_CLASS(USphereComponent, UShapeComponent)
	REFLECT_START(ClassName)
		PROPERTY(SphereRadius)
		REFLECT_END()
public:
	float SphereRadius = 0.5f;

	// 비균일 스케일이면 가장 큰 축을 쓴다
	float GetScaledSphereRadius() const;
	virtual FBox CalcLocalBounds() const override;
	virtual void DrawDebug(FLineBatcher*, const FVector4&) const override;
};
