#include "EnginePCH.h"
#include "LevelSequence/LevelSequenceActor.h"

#include "LevelSequence/LevelSequence.h"
#include "LevelSequence/LevelSequencePlayer.h"
#include "UObject/UObjectGlobals.h"

ALevelSequenceActor::ALevelSequenceActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

ALevelSequenceActor::~ALevelSequenceActor()
{
	ReleasePlayer();
}

FMovieSceneSequencePlaybackSettings ALevelSequenceActor::MakePlaybackSettings() const
{
	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = bAutoPlay;
	Settings.LoopCount.Value = LoopCount;
	Settings.PlayRate = PlayRate;
	Settings.StartTime = StartTime;
	Settings.bDisableCameraCuts = bDisableCameraCuts;
	Settings.bPauseAtEnd = bPauseAtEnd;
	Settings.FinishCompletionStateOverride = bRestoreState
		? EMovieSceneCompletionModeOverride::ForceRestoreState
		: EMovieSceneCompletionModeOverride::ForceKeepState;
	return Settings;
}

void ALevelSequenceActor::ApplyPlaybackSettings(const FMovieSceneSequencePlaybackSettings& Settings)
{
	bAutoPlay = Settings.bAutoPlay;
	LoopCount = Settings.LoopCount.Value;
	PlayRate = Settings.PlayRate;
	StartTime = Settings.StartTime;
	bDisableCameraCuts = Settings.bDisableCameraCuts;
	bPauseAtEnd = Settings.bPauseAtEnd;
	bRestoreState = Settings.FinishCompletionStateOverride != EMovieSceneCompletionModeOverride::ForceKeepState;
}

bool ALevelSequenceActor::InitializePlayer()
{
	ReleasePlayer();
	LevelSequence = ULevelSequence::LoadLevelSequence(LevelSequenceAsset);
	if (!LevelSequence) return false;
	SequencePlayer = NewObject<ULevelSequencePlayer>(this);
	SequencePlayer->Initialize(LevelSequence, GetWorld(), MakePlaybackSettings());
	return true;
}

void ALevelSequenceActor::ReleasePlayer()
{
	delete SequencePlayer;
	SequencePlayer = nullptr;
	delete LevelSequence;
	LevelSequence = nullptr;
}

void ALevelSequenceActor::BeginPlay()
{
	Super::BeginPlay();
	if (!SequencePlayer) InitializePlayer();
	if (SequencePlayer && bAutoPlay) SequencePlayer->Play();
}

void ALevelSequenceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// DeltaTime은 월드 Time Dilation이 적용된 값 (UE도 월드 게임 시간으로 진행)
	if (SequencePlayer) SequencePlayer->Update(DeltaTime);
}

void ALevelSequenceActor::EndPlay()
{
	// 월드가 정리되는 중이라 다른 액터가 먼저 지워졌을 수 있다. 되돌리지 않고 버린다
	if (SequencePlayer) SequencePlayer->DiscardStateAndStop();
	Super::EndPlay();
}
