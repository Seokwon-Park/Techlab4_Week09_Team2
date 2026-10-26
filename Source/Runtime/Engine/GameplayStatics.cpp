#include "EnginePCH.h"
#include "GameplayStatics.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

FString UGameplayStatics::ResolveLevelPath(const FString& LevelName)
{
	// 경로가 들어 있으면 그대로 쓴다.
	if (LevelName.find('/') != FString::npos || LevelName.find('\\') != FString::npos)
		return LevelName;
	constexpr const char* Ext = ".scene";
	const bool bHasExt = LevelName.size() >= 6 && LevelName.compare(LevelName.size() - 6, 6, Ext) == 0;
	return "Scenes/" + LevelName + (bHasExt ? "" : Ext);
}


void UGameplayStatics::OpenLevel(UWorld* World, const FString& LevelName)
{
	if (!GEngine || !World || LevelName.empty()) return;
	GEngine->SetClientTravel(World, ResolveLevelPath(LevelName));
}

UGameInstance* UGameplayStatics::GetGameInstance(const UWorld* World)
{
	FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;
	return Context ? Context->OwningGameInstance : nullptr;
}
