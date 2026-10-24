#pragma once

#include "UI/Widget.h"

class UFont;

class UTextBlock : public UWidget
{
	DECLARE_CLASS(UTextBlock, UWidget)

	REFLECT_START(ClassName)
		PROPERTY(Text)
		PROPERTY(FontPath)
		PROPERTY(FontSize)
		PROPERTY(ColorAndOpacity)
		PROPERTY(Justification)
	REFLECT_END()

public:
	UTextBlock();

	void SetText(const FString& InText) { Text = InText; }
	const FString& GetText() const { return Text; }

	virtual FVector2 GetDesiredSize() const override;
	virtual void Paint(FUIDrawList& DrawList, float ParentOpacity) const override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;
	virtual void OnLoaded() override { ResolveFont(); }

	FString Text = "Text Block";
	FString FontPath = "Assets/Fonts/Pretendard.json";
	float FontSize = 32.0f;                              // 디자인 해상도 기준 픽셀
	FLinearColor ColorAndOpacity = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	int32 Justification = 0;                             // 0 왼쪽, 1 가운데, 2 오른쪽

private:
	void ResolveFont();

	UFont* Font = nullptr;
};
