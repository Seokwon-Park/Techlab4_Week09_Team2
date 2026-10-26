#pragma once

#include "BoxComponent.h"

// 빙판의 이동 영역. 바닥 메시와 별도로 플레이어 높이를 덮도록 배치한다.
class UIceVolumeComponent : public UBoxComponent
{
	DECLARE_CLASS(UIceVolumeComponent, UBoxComponent)
	REFLECT_START(ClassName)
		PROPERTY(IceType)
	REFLECT_END()

public:
	UIceVolumeComponent()
	{
		ShapeColor = FColor(100, 190, 255, 255);
		bBlockComponent = false;
	}

	FString GetIceType() const { return IceType; }
	void SetIceType(const FString& InIceType);  
	 
private:
	FString IceType = "default"; // default, slippery, reverse
};
