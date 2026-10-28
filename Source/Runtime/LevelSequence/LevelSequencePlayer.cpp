#include "EnginePCH.h"
#include "LevelSequence/LevelSequencePlayer.h"

#include "LevelSequence/LevelSequence.h"
#include "LevelSequence/LevelSequenceActor.h"
#include "MovieSceneTracks/MovieScenePropertyBinding.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"

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

	// UE는 PlayerController->SetViewTarget(CameraActor, BlendParams). 이 엔진은 PCM이 뷰를 아직 만들지 않으므로
	// 게임 뷰포트 카메라를 바꾼다. nullptr이면 다음 CalcSceneView가 플레이어 카메라를 다시 찾는다
	if (World && World->IsGameWorld() && GEngine)
	{
		if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(World); Context && Context->GameViewport)
			Context->GameViewport->SetCameraComponent(Camera);
	}
	OnCameraCut.Broadcast(Camera);
}
