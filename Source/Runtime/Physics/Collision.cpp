#include "EnginePCH.h"
#include "Collision.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

namespace
{
	FVector Row(const FMatrix& M, int32 R) { return FVector(M.M[R][0], M.M[R][1], M.M[R][2]); }
	
	// 정렬 순서가 곧 분기 규칙: 항상 작은 쪽이 앞에 오게 바꾸면 조합이 9개가 아니라 6개가 된다
	enum class EShapeKind : int32 { Sphere = 0, Capsule = 1, Box = 2, Unknown = 3 };

	EShapeKind KindOf(const UShapeComponent* S)
	{
		UClass* C = S->GetClass();
		if (C->IsChildOf(USphereComponent::StaticClass()))  return EShapeKind::Sphere;
		if (C->IsChildOf(UCapsuleComponent::StaticClass())) return EShapeKind::Capsule;
		if (C->IsChildOf(UBoxComponent::StaticClass()))     return EShapeKind::Box;
		return EShapeKind::Unknown;
	}
}

namespace FCollision
{
	bool SphereSphere(const FWorldSphere& A, const FWorldSphere& B)
	{
		const float R = A.Radius + B.Radius;
		return (A.Center - B.Center).SizeSquared() <= R * R;   // 제곱끼리 비교해서 sqrt를 피한다
	}

	bool Overlap(const UShapeComponent* A, const UShapeComponent* B)
	{
		if (!A || !B || A == B)
			return false;

		EShapeKind KA = KindOf(A), KB = KindOf(B);
		if (KA == EShapeKind::Unknown || KB == EShapeKind::Unknown)
			return false;
		if (KA > KB) { std::swap(A, B); std::swap(KA, KB); }

		auto AsSphere = [](const UShapeComponent* S) { return ToWorld(static_cast<const USphereComponent*>(S)); };
		auto AsCapsule = [](const UShapeComponent* S) { return ToWorld(static_cast<const UCapsuleComponent*>(S)); };
		auto AsBox = [](const UShapeComponent* S) { return ToWorld(static_cast<const UBoxComponent*>(S)); };

		switch (KA)
		{
		case EShapeKind::Sphere:
			if (KB == EShapeKind::Sphere)  return SphereSphere(AsSphere(A), AsSphere(B));
			/*if (KB == EShapeKind::Capsule) return CapsuleSphere(AsCapsule(B), AsSphere(A));
			return SphereBox(AsSphere(A), AsBox(B));*/
		/*case EShapeKind::Capsule:
			if (KB == EShapeKind::Capsule) return CapsuleCapsule(AsCapsule(A), AsCapsule(B));
			return CapsuleBox(AsCapsule(A), AsBox(B));
		default:
			return BoxBox(AsBox(A), AsBox(B));*/
		}
	}

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
		int32 Failed = 0;
		auto Expect = [&](bool Got, bool Want, const char* Name)
			{
				if (Got != Want) { ++Failed; HTR_LOG(Error, "[Collision] FAIL {} (got {}, want {})", Name, Got, Want); }
			};

		const FVector Zero(0, 0, 0);
		const FWorldBox UnitBox{ Zero, { FVector(1,0,0), FVector(0,1,0), FVector(0,0,1) }, FVector(1, 1, 1) };
		const float S45 = std::sqrt(0.5f);
		auto RotatedBox = [&](const FVector& C)   // Z축으로 45도 돌린 반폭 1 박스
			{ return FWorldBox{ C, { FVector(S45, S45, 0), FVector(-S45, S45, 0), FVector(0,0,1) }, FVector(1, 1, 1) }; };
		auto Capsule = [](float X, float Y) { return FWorldCapsule{ FVector(X, Y, 1), FVector(X, Y, -1), 0.5f }; };   // Z축으로 선 캡슐

		// 구-구: 반지름 1+1, 중심 거리 1.9 / 2.1
		Expect(SphereSphere({ Zero, 1 }, { FVector(1.9f, 0, 0), 1 }), true, "sphere-sphere touch");
		Expect(SphereSphere({ Zero, 1 }, { FVector(2.1f, 0, 0), 1 }), false, "sphere-sphere apart");

		HTR_LOG(Info, "[Collision] self test: {} failed", Failed);
		return Failed;
	}
}