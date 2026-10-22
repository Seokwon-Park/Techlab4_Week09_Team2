#pragma once

#include "PrimitiveComponent.h"

// 충돌 판정용 도형의 공통 부모. 메시가 없고 LineBatcher로 와이어프레임만 그린다. (담당: D)
class UShapeComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)
	REFLECT_START(ClassName)
		PROPERTY(ShapeColor)
		PROPERTY(bDrawOnlyIfSelected)
		REFLECT_END()
public:
	UShapeComponent();

	// 넓은 범위 판정용. 월드 스케일과 회전이 반영된 AABB
	virtual FBox GetWorldAABB() const;

	FColor ShapeColor = FColor(255, 0, 0, 255);
	bool bDrawOnlyIfSelected = false;

protected:
	// TODO(D): 디버그 그리기를 붙이면 LineBatcher 경로로 바꾼다. 메시가 없으므로 프록시를 만들지 않는다.
	virtual bool ShouldCreateRenderState() const override { return false; }
};
