#pragma once

#include "UI/Widget.h"

class UTexture2D;

// 텍스처가 없으면 단색 판으로 그린다 (점수판 배경에 사용).
class UImage : public UWidget
{
	DECLARE_CLASS(UImage, UWidget)

	REFLECT_START(ClassName)
		PROPERTY(TexturePath)
		PROPERTY(ColorAndOpacity)
	REFLECT_END()

public:
	virtual void Paint(FUIDrawList& DrawList, float ParentOpacity) const override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;
	virtual void OnLoaded() override { ResolveTexture(); }

	FString TexturePath;
	FLinearColor ColorAndOpacity = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

private:
	void ResolveTexture();

	UTexture2D* Texture = nullptr;
};
