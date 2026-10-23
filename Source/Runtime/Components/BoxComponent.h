#pragma once

#include "ShapeComponent.h"

class UBoxComponent : public UShapeComponent
{
	DECLARE_CLASS(UBoxComponent, UShapeComponent)
	REFLECT_START(ClassName)
		PROPERTY(BoxExtent)
		REFLECT_END()
public:
	// 로컬 공간 반 크기. 월드 크기는 GetScaledBoxExtent
	FVector BoxExtent = FVector(0.5f, 0.5f, 0.5f);

	FVector GetScaledBoxExtent() const;
	virtual FBox CalcLocalBounds() const override;
	virtual void DrawDebug(FLineBatcher*, const FVector4&) const override;
};
