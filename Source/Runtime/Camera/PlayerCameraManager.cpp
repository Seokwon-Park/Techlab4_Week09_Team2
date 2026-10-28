#include "EnginePCH.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraModifier.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

// =======================FMinimalViewInfo===============
FMatrix FMinimalViewInfo::CalculateViewMatrix() const
{
	const FMatrix InverseTranslation = FMatrix::MakeTranslation(Location * -1.0f);
	const FMatrix InverseRotation = Rotation.Quaternion().ToFMatrix().GetTransposed();
	return InverseTranslation * InverseRotation;
}

FMatrix FMinimalViewInfo::CalculateProjectionMatrix() const
{
	if (bIsOrthogonal)
	{
		const float Width = OrthoWidth;
		const float Height = Width / AspectRatio;

		const float XScale = 2.0f / Width;
		const float YScale = 2.0f / Height;

		const float ZScale = 1.0f / (FarClip - NearClip);
		const float ZOffset = -NearClip / (FarClip - NearClip);

		return FMatrix(
			0.0f, 0.0f, ZScale, 0.0f,
			XScale, 0.0f, 0.0f, 0.0f,
			0.0f, YScale, 0.0f, 0.0f,
			0.0f, 0.0f, ZOffset, 1.0f
		);
	}

	const float HalfFOV = FMath::DegreesToRadians(FOV) * 0.5f;

	const float YScale = 1.0f / tan(HalfFOV);
	const float XScale = YScale / AspectRatio;

	const float ZScale = FarClip / (FarClip - NearClip);
	const float ZOffset = -NearClip * FarClip / (FarClip - NearClip);

	return FMatrix(
		0.0f, 0.0f, ZScale, 1.0f,
		XScale, 0.0f, 0.0f, 0.0f,
		0.0f, YScale, 0.0f, 0.0f,
		0.0f, 0.0f, ZOffset, 0.0f
	);
}

void FMinimalViewInfo::BlendViewInfo(const FMinimalViewInfo& Other, float Alpha)
{
	Location = FMath::Lerp(Location, Other.Location, Alpha);
	Rotation = FMath::LerpRotator(Rotation, Other.Rotation, Alpha);
	FOV = FMath::Lerp(FOV, Other.FOV, Alpha);
	OrthoWidth = FMath::Lerp(OrthoWidth, Other.OrthoWidth, Alpha);
	NearClip = FMath::Lerp(NearClip, Other.NearClip, Alpha);
	FarClip = FMath::Lerp(FarClip, Other.FarClip, Alpha);
	if (Alpha >= 0.5f) bIsOrthogonal = Other.bIsOrthogonal;
}
// =======================FViewTargetTransitionParams===============
float FViewTargetTransitionParams::GetBlendAlpha(float TimePct) const
{
	const float T = FMath::Clamp(TimePct, 0.0f, 1.0f);
	// 2. 곡선 고르기
	switch (BlendFunction)
	{
	case EViewTargetBlendFunction::Linear:    return T;
	//case EViewTargetBlendFunction::Cubic:     return FMath::CubicInterp();
	case EViewTargetBlendFunction::SmoothStep:return FMath::CubicInterp(0.0f,0.0f,1.0f,0.0f,T);
	case EViewTargetBlendFunction::EaseIn:    return pow(T,BlendExp);
	case EViewTargetBlendFunction::EaseOut:   return 1-pow(1-T,BlendExp);
	case EViewTargetBlendFunction::EaseInOut: return FMath::InterpEaseInOut(0.0f, 1.0f, T, BlendExp);
	// 입맞에 맞게 커스텀해서 만들면됨.
	// 헤더파일의 EViewTargetBlendFunction에서 enum 선언필요
	}
	return T;
}

//// =======================APlayerCameraManager===============
void APlayerCameraManager::InitializeFor(APlayerController* PC)
{
	PlayerControllerOwner = PC;
	SetViewTarget(PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr);
}

void APlayerCameraManager::SetViewTarget(AActor* NewTarget, const FViewTargetTransitionParams& Params)
{
	if (!NewTarget) NewTarget = PlayerControllerOwner;
	if (NewTarget == PendingViewTarget.Target) return;
	if (NewTarget == ViewTarget.Target && !PendingViewTarget.Target) return;

	BlendParams = Params;
	if (Params.BlendTime > 0.0f)
	{
		// 전환중 재전환 : 지금 화면을 출발점으로 고정
		if (PendingViewTarget.Target && bCameraCacheValid)
		{
			ViewTarget.POV = CameraCachePOV;
			bFreezeOutgoing = true;
		}
		PendingViewTarget.SetTarget(NewTarget);
		BlendTimeToGo = Params.BlendTime;
	}
	else
	{
		ViewTarget.SetTarget(NewTarget);
		PendingViewTarget.SetTarget(nullptr);
		bFreezeOutgoing = false;
	}		

}


void APlayerCameraManager::UpdateCamera(float DeltaTime) 
{
	//현재 대상 시점
	const bool bOutgoingLocked = bFreezeOutgoing || (PendingViewTarget.Target && BlendParams.bLockOutgoing);
	if (!bOutgoingLocked)
	{
		CheckViewTarget(ViewTarget);
		UpdateViewTarget(ViewTarget, DeltaTime);
	}
	FMinimalViewInfo NewPOV = ViewTarget.POV;
	
	// 전환 
	if (PendingViewTarget.Target)
	{
		CheckViewTarget(PendingViewTarget);
		UpdateViewTarget(PendingViewTarget, DeltaTime);
		BlendTimeToGo -= DeltaTime;
		if (BlendTimeToGo > 0.0f)
		{
			const float TimePct = 1.0f - BlendTimeToGo / BlendParams.BlendTime;
			NewPOV.BlendViewInfo(PendingViewTarget.POV, BlendParams.GetBlendAlpha(TimePct));
		}
		else
		{
			//  전환 완료: Pending → ViewTarget
			ViewTarget = PendingViewTarget;
			PendingViewTarget.SetTarget(nullptr);
			bFreezeOutgoing = false;
			BlendTimeToGo = 0.0f;
			NewPOV = ViewTarget.POV;
		}
	}
	//효과
	ApplyCameraModifiers(DeltaTime, NewPOV);
	
	//Fade
	
	//결과저장
	CameraCachePOV = NewPOV;
	bCameraCacheValid = true;
}

void APlayerCameraManager::AddCameraModifier(UCameraModifier* Modifier)
{
	if (!Modifier || ModifierList.Find(Modifier) >= 0 ||
		(Modifier->GetCameraOwner() && Modifier->GetCameraOwner() != this)) return;
	Modifier->AddedToCamera(this);
	ModifierList.Add(Modifier);
}

void APlayerCameraManager::RemoveCameraModifier(UCameraModifier* Modifier)
{
	const int32 Index = ModifierList.Find(Modifier);
	if (Index < 0) return;
	ModifierList.RemoveAt(Index);
	delete Modifier;
}

UCameraModifier* APlayerCameraManager::FindCameraModifierByClass(UClass* ModifierClass) const
{
	if (!ModifierClass) return nullptr;
	for (UCameraModifier* Modifier : ModifierList)
		if (Modifier && Modifier->IsA(ModifierClass)) return Modifier;
	return nullptr;
}

UCameraShakeBase* APlayerCameraManager::StartCameraShake(UClass* ShakeClass, float Scale)
{
	auto* Modifier = static_cast<UCameraModifier_CameraShake*>(
		FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass()));
	if (!Modifier)
	{
		Modifier = NewObject<UCameraModifier_CameraShake>(this);
		AddCameraModifier(Modifier);
	}
	return Modifier ? Modifier->AddCameraShake(ShakeClass, Scale) : nullptr;
}

void APlayerCameraManager::StopCameraShake(UCameraShakeBase* ShakeInstance, bool bImmediately)
{
	auto* Modifier = static_cast<UCameraModifier_CameraShake*>(
		FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass()));
	if (Modifier) Modifier->RemoveCameraShake(ShakeInstance, bImmediately);
}

void APlayerCameraManager::StopAllCameraShakes(bool bImmediately)
{
	auto* Modifier = static_cast<UCameraModifier_CameraShake*>(
		FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass()));
	if (Modifier) Modifier->RemoveAllCameraShakes(bImmediately);
}

UCameraModifier_FOVPunch* APlayerCameraManager::PlayFOVPunch(UCurveFloat* Curve, float Duration, float Strength)
{
	auto* Modifier = static_cast<UCameraModifier_FOVPunch*>(
		FindCameraModifierByClass(UCameraModifier_FOVPunch::StaticClass()));
	if (!Modifier)
	{
		Modifier = NewObject<UCameraModifier_FOVPunch>(this);
		AddCameraModifier(Modifier);
	}
	if (!Modifier) return nullptr;
	Modifier->Play(Curve, Duration, Strength);
	return Modifier->IsPlaying() ? Modifier : nullptr;
}

void APlayerCameraManager::StopFOVPunch(bool bImmediately)
{
	auto* Modifier = static_cast<UCameraModifier_FOVPunch*>(
		FindCameraModifierByClass(UCameraModifier_FOVPunch::StaticClass()));
	if (Modifier) Modifier->Stop(bImmediately);
}

bool APlayerCameraManager::IsActorAlive(AActor* Actor, uint32 UUID) const
{
	// 1. 레벨에 같은 주소가 있고 UUID가 같으면 살아 있음
	UWorld* World = GetWorld();
	if (!World || !Actor) return false;
	for (ULevel* Level : World->GetLevels())
		for (AActor* A : Level->GetActors())
			if (A == Actor) return A->GetUUID() == UUID;
	return false;
}

void APlayerCameraManager::CheckViewTarget(FViewTarget& VT)
{
	// 1. 살아 있으면 그대로
	if (IsActorAlive(VT.Target, VT.TargetUUID)) return;
	// 2. Pawn → PC 순서로 대체
	AActor* Fallback = nullptr;
	if (PlayerControllerOwner) Fallback = PlayerControllerOwner->GetPawn()
		? static_cast<AActor*>(PlayerControllerOwner->GetPawn()) : PlayerControllerOwner;
	VT.SetTarget(Fallback);
}

void APlayerCameraManager::UpdateViewTarget(FViewTarget& OutVT, float DeltaTime)
{
	if (OutVT.Target) OutVT.Target->CalcCamera(DeltaTime, OutVT.POV);
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	std::stable_sort(ModifierList.begin(), ModifierList.end(),
		[](const auto* A, const auto* B) { return A->Priority < B->Priority; });
	for (UCameraModifier* Modifier : ModifierList)
		if (!Modifier->IsDisabled() && Modifier->ModifyCamera(DeltaTime, InOutPOV))
			break;
	InOutPOV.FOV = std::isfinite(InOutPOV.FOV) ? std::clamp(InOutPOV.FOV, 1.0f, 179.0f) : 60.0f;
}

