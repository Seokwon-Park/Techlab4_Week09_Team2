#pragma once

#include "UI/UserWidget.h"

class UTextBlock;

class UScoreboardWidget : public UUserWidget
{
	DECLARE_CLASS(UScoreboardWidget, UUserWidget)

public:
	void SetScore(int32 InScore);
	void SetTimeLeft(float Seconds);

protected:
	// 배치는 Assets/UI/Scoreboard.widget에서 온다. 여기서는 이름으로 위젯만 찾는다.
	virtual void NativeConstruct() override;

private:
	UTextBlock* ScoreText = nullptr;
	UTextBlock* TimeText = nullptr;
};
