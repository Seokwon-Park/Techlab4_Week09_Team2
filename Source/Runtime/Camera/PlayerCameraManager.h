#pragma once
#include "GameFramework/Actor.h"

#include "PlayerCameraManager.generated.h" // 왜이렇게 해야하는지 고민필요

struct FMinimalViewInfo
{
	FVector Location;
	FRotator Rotation;
	float FOV = 60.0f;
	float AspectRatio = 16.0f / 9.0f;
	float OrthoWidth = 16.0f;
	float NearClip = 0.1f;
	float FarClip = 10000.0f;
	bool bIsOrthogonal = false;

	void BlendViewInfo(const FMinimalViewInfo& Other, float Alpha);
	FMatrix CalculateViewMatrix() const;
	FMatrix CalculateProjectionMatrix() const;
};

struct FViewTarget
{
	AActor* Target = nullptr;
	uint32 TargetUUID = 0; // 파괴감지용
	FMinimalViewInfo POV;

	void SetTarget(AActor* InTarget)
	{
		Target = InTarget;
		TargetUUID = InTarget ? InTarget->GetUUID() : 0;
	}
};

enum class EViewTargetBlendFunction : uint8
{
	Linear,// alpha = t
	Cubic, // alpha = 3t^2 - 2t^3
	EaseIn, // alpha = t^e
	EaseOut, // 1- (1-t)^e
	EaseInOut,
	SmoothStep,
};

struct FViewTargetTransitionParams
{
	float BlendTime = 0.0f;
	float BlendExp = 2.0f;
	EViewTargetBlendFunction BlendFunction = EViewTargetBlendFunction::Cubic;
	bool bLockOutgoing = false;
	float GetBlendAlpha(float TimePct) const;

};
class UCameraModifier;
class APlayerController;

UCLASS()
class APlayerCameraManager : public AActor
{
	GENERATED_BODY()
public:
	APlayerCameraManager() {};
	~APlayerCameraManager() {};

	void AddCameraModifier(UCameraModifier* Modifier);
	void RemoveCameraModifier(UCameraModifier* Modifier);
	UCameraModifier* FindCameraModifierByClass(UClass* ModifierClass) const;

	void StartCameraFade();
	void StopCameraFade();
	void SetManualCameraFade(float InFadeAmount, const FLinearColor& InFadeColor, bool bInFadeAudio = false, bool bInFadeVideo = false);

	const FViewTarget& GetViewTarget() const { return ViewTarget; }
	const FMinimalViewInfo& GetCameraCachePOV() const { return CameraCachePOV; }
	
	bool HasCameraCache() const { return bCameraCacheValid; }
	bool IsActorAlive(AActor* Actor, uint32 UUID) const;
	bool IsBlending() const { return PendingViewTarget.Target != nullptr; }


	// 이 액터의 눈으로 카메라를 보겠다.
	void SetViewTarget(AActor* NewTarget, const FViewTargetTransitionParams& Params = {});
	void UpdateViewTarget(FViewTarget& OutVT, float DeltaTime);
	void UpdateCamera(float DeltaTime);
	void InitializeFor(APlayerController* PC);
	void CheckViewTarget(FViewTarget& VT);
	void ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV);

	APlayerController* GetOwningPlayerController() const { return PlayerControllerOwner; }

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
	APlayerController* PlayerControllerOwner = nullptr;
	FMinimalViewInfo CameraCachePOV;
	bool bCameraCacheValid = false;

	FViewTarget PendingViewTarget;
	FViewTargetTransitionParams BlendParams;
	float BlendTimeToGo = 0.0f;
	bool  bFreezeOutgoing = false;
};