#include "EnginePCH.h"
#include "LevelSequence/LevelSequencePlayer.h"

#include "LevelSequence/LevelSequence.h"
#include "LevelSequence/LevelSequenceActor.h"
#include "MovieSceneTracks/MovieScenePropertyBinding.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"

void ULevelSequencePlayer::Initialize(ULevelSequence* InLevelSequence, UWorld* InWorld, const FMovieSceneSequencePlaybackSettings& InSettings)
{
	UMovieSceneSequencePlayer::Initialize(InLevelSequence, InWorld, InSettings);
}

ULevelSequencePlayer* ULevelSequencePlayer::CreateLevelSequencePlayer(UWorld* World, const FString& LevelSequencePath,
	const FMovieSceneSequencePlaybackSettings& Settings, ALevelSequenceActor*& OutActor)
{
	OutActor = nullptr;
	if (!World) return nullptr;
	ALevelSequenceActor* Actor = World->SpawnActor<ALevelSequenceActor>();
	if (!Actor) return nullptr;
	Actor->LevelSequenceAsset = LevelSequencePath;
	Actor->ApplyPlaybackSettings(Settings);
	Actor->InitializePlayer();
	OutActor = Actor;
	return Actor->GetSequencePlayer();
}

UCameraComponent* ULevelSequencePlayer::GetActiveCameraComponent() const
{
	return MovieSceneHelpers::CameraComponentFromRuntimeObject(CurrentCameraObject);
}

void ULevelSequencePlayer::UpdateCameraCut(UObject* CameraObject, float BlendTime)
{
	UCameraComponent* Camera = MovieSceneHelpers::CameraComponentFromRuntimeObject(CameraObject);

	if (World && World->IsGameWorld())
	{
		// UE MovieSceneCameraCutTrackInstance: PlayerController->SetViewTarget(CameraActor, BlendParams).
		// 컷이 풀리면(nullptr) 조종 중인 Pawn으로 돌려준다
		APlayerController* PC = World->GetPlayerController();
		if (APlayerCameraManager* PCM = PC ? PC->GetCameraManager() : nullptr)
		{
			AActor* NewTarget = Camera ? Camera->GetOwner() : static_cast<AActor*>(PC->GetPawn());
			FViewTargetTransitionParams Params;
			Params.BlendTime = std::max(BlendTime, 0.0f);
			PCM->SetViewTarget(NewTarget, Params);
		}
		// PCM이 없으면 게임 뷰포트 카메라를 직접 바꾼다. nullptr이면 다음 CalcSceneView가 플레이어 카메라를 다시 찾는다
		else if (FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr; Context && Context->GameViewport)
		{
			Context->GameViewport->SetCameraComponent(Camera);
		}
	}
	OnCameraCut.Broadcast(Camera);
}
