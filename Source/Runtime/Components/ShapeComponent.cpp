#include "EnginePCH.h"
#include "ShapeComponent.h"

UShapeComponent::UShapeComponent()
{
	bCastShadow = false;
	bReceiveShadow = false;
}

FBox UShapeComponent::GetWorldAABB() const
{
	// 자식이 CalcLocalBounds를 채우면 월드 변환은 SceneComponent가 처리한다
	return CalcBounds();
}
