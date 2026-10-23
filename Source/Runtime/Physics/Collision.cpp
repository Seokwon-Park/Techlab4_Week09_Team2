#include "EnginePCH.h"
#include "Collision.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

namespace
{
	FVector Row(const FMatrix& M, int32 R) { return FVector(M.M[R][0], M.M[R][1], M.M[R][2]); }
}

namespace FCollision
{
	FWorldSphere ToWorld(const USphereComponent* S)
	{
		return { S->GetWorldLocation(), S->GetScaledSphereRadius() };
	}

	FWorldCapsule ToWorld(const UCapsuleComponent* C)
	{
		const FMatrix M = C->GetWorldMatrix();
		const FVector Center = Row(M, 3);
		const FVector Up = Row(M, 2).GetSafeNormal();
		const float Radius = C->GetScaledCapsuleRadius();
		const float Segment = C->GetScaledCapsuleHalfHeight() - Radius;   // 반구 중심까지 거리 (0이면 구)
		return { Center + Up * Segment, Center - Up * Segment, Radius };
	}

	FWorldBox ToWorld(const UBoxComponent* B)
	{
		const FMatrix M = B->GetWorldMatrix();
		FWorldBox Box;
		Box.Center = Row(M, 3);
		for (int32 i = 0; i < 3; ++i)
			Box.Axis[i] = Row(M, i).GetSafeNormal();   // 스케일은 Extent 쪽에 들어간다
		Box.Extent = B->GetScaledBoxExtent();
		return Box;
	}
	int32 RunSelfTest()
	{
		return int32();
	}
}