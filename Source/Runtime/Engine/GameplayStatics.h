#pragma once

#include "Core/EngineString.h"

class UWorld;
class UGameInstance;

struct UGameplayStatics
{
	static void OpenLevel(UWorld* World, const FString& LevelName);
	static UGameInstance* GetGameInstance(const UWorld* World);
	static FString ResolveLevelPath(const FString& LevelName);
};