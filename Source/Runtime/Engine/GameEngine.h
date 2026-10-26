#pragma once

#include "Engine.h"
#include "Engine/GameSettings.h"

#include "UI/UIRenderer.h"
#include "UI/UIDrawList.h"

class FGameViewportClient;

class UGameEngine : public UEngine
{
	DECLARE_CLASS(UGameEngine, UEngine)
public:
	FEngineConfig GetConfig() const override;

	bool Init() override;
	void Tick(float DeltaTime) override;
	void PreExit() override;

	// 현재 Game 월드를 버리고 씬 파일로 새 월드를 연다.
	bool LoadMap(const FString& MapPath);
	// 게임 로직(Lua 등)에서 레벨을 바꿀 때 쓴다. 월드 Tick 도중 월드를 지우지 않도록 다음 프레임에 연다.
	void RequestLoadMap(const FString& MapPath) { PendingMap = MapPath; }
private:
	void DispatchInput(FGameViewportClient& Viewport, float DeltaTime);
	void RenderFrame(FGameViewportClient& Viewport, UWorld* World);

	// GetConfig는 창을 만들기 전에 const로 불리므로 거기서 읽어 둔다.
	mutable FGameSettings Settings;
	FString PendingMap;

	TUniquePtr<FUIRenderer> UIRenderer;
	FUIDrawList UIDrawList;
};