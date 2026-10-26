#pragma once

#include "MovementComponent.h"

// XY 평면 이동. 빙판에서는 관성을 유지하며 조향 방향으로 서서히 접지한다.
class USlidingMovementComponent : public UMovementComponent
{
	DECLARE_CLASS(USlidingMovementComponent, UMovementComponent)
	REFLECT_START(ClassName)
		PROPERTY(MoveSpeed)
		PROPERTY(Acceleration)
		PROPERTY(Deceleration)
		PROPERTY(TurnRate)
		PROPERTY(IceGrip)
		PROPERTY(CollisionRadius)
	REFLECT_END()

public:
	USlidingMovementComponent();
	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;
	void HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta) override;

	// 매 프레임 입력을 전달한다. 입력이 없으면 ZeroVector를 전달한다.
	void SetMoveInput(const FVector& Direction);
	// 빙판 여부는 Lua가 기존 Overlap 이벤트를 받아 결정한다.
	void SetOnIce(bool bInOnIce) { bOnIce = bInOnIce; }
	bool IsOnIce() const { return bOnIce; }
	void SetMovementEnabled(bool bEnabled);
	bool IsMovementEnabled() const { return bMovementEnabled; }
	void ResetMovement();
	FVector GetSteeringDirection() const { return SteeringDirection; }

	float MoveSpeed = 6.0f; // 최대 속력
	float Acceleration = 8.0f; // world units/second^2
	float Deceleration = 18.0f; // 일반 바닥의 감속도
	float TurnRate = 180.0f; // degrees/second
	float IceGrip = 1.5f; // 1/second. 낮을수록 옆으로 오래 미끄러진다.
	float CollisionRadius = 0.5f;

private:
	FVector InputDirection = FVector::ZeroVector;
	FVector SteeringDirection = FVector::ZeroVector;
	bool bOnIce = false;
	bool bMovementEnabled = true;
};
