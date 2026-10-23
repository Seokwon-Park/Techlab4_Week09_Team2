#pragma once

#include "ShapeComponent.h"

// 로컬 Z축이 캡슐 축이다. HalfHeight는 반구 끝까지 포함한 길이 (UE와 같음). 담당: D
class UCapsuleComponent : public UShapeComponent
{
	DECLARE_CLASS(UCapsuleComponent, UShapeComponent)
	REFLECT_START(ClassName)
		PROPERTY(CapsuleHalfHeight)
		PROPERTY(CapsuleRadius)
		REFLECT_END()
public:
	float CapsuleHalfHeight = 1.0f;
	float CapsuleRadius = 0.5f;

	float GetScaledCapsuleHalfHeight() const;
	float GetScaledCapsuleRadius() const;
	virtual FBox CalcLocalBounds() const override;
	virtual void DrawDebug(FLineBatcher*, const FVector4&) const override;
};
