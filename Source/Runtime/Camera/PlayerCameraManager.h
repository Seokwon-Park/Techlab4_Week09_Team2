#pragma once
#include "GameFramework/Actor.h"

struct FMinimalViewInfo
{
	FVector Location;
	FRotator Rotation;
	float FOV = 60.0f;

	// AspectRatio, OrthoWidth, Projection type, post process settings, etc.
};

struct FViewTarget
{
	AActor* Target = nullptr;
	FMinimalViewInfo POV;
};


class UCameraModifier;
class APlayerCameraManager : public AActor
{
public:
	APlayerCameraManager();
	~APlayerCameraManager();

	void AddCameraModifier(UCameraModifier* Modifier);
	void RemoveCameraModifier(UCameraModifier* Modifier);
	UCameraModifier* FindCameraModifierByClass(UClass* ModifierClass) const;

	void StartCameraFade();
	void StopCameraFade();
	void SetManualCameraFade(float InFadeAmount, const FLinearColor& InFadeColor, bool bInFadeAudio = false, bool bInFadeVideo = false);

	const FViewTarget& GetViewTarget() const { return ViewTarget; }
	void SetViewTarget(AActor* NewTarget, const FViewTarget& NewViewTarget);

public:
	FLinearColor FadeColor;
	float FadeAmount;
	FVector2 FadeAlpha;
	float FadeTime;
	float FadeTimeRemaining;

	FName CameraStyle;
	FViewTarget ViewTarget;

protected:
	TArray<UCameraModifier*> ModifierList;
};