#include "EnginePCH.h"
#include "SlidingMovementComponent.h"
#include "Engine/World.h"
#include <cmath>

USlidingMovementComponent::USlidingMovementComponent()
{
	Velocity = FVector::ZeroVector;
	PrimaryComponentTick.bCanEverTick = true;
	// Actor의 Lua 입력이 갱신된 다음 이동한다.
	PrimaryComponentTick.TickGroup = ETickingGroup::DuringPhysics;
}

USlidingMovementComponent::~USlidingMovementComponent()
{
	OnDeath.Remove(DeathNotifyHandle);
}

void USlidingMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UpdatedComponent) SpawnLocation = UpdatedComponent->GetWorldLocation();
	OnDeath.Remove(DeathNotifyHandle);
	DeathNotifyHandle = OnDeath.AddDynamic(GetOwner(), &AActor::NotifyDeath);
	bOnIce = false;
}

void USlidingMovementComponent::SetMoveInput(const FVector& Direction)
{
	InputDirection = FVector(Direction.X, Direction.Y, 0.0f).GetClampedToMaxSize(1.0f);
}

void USlidingMovementComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	if (bDead || !UpdatedComponent || !GetWorld() || !GetWorld()->IsGameWorld() ||
		DeltaTime <= 0.0f || !std::isfinite(DeltaTime)) return;

	const float Speed = std::max(MoveSpeed, 0.0f);
	const float Accel = std::max(Acceleration, 0.0f);
	const bool bHasInput = InputDirection.SizeSquared() > SMALL_NUMBER;
	const FVector OldVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
	Velocity = OldVelocity;
	if (!bOnIce)
	{
		// 일반 바닥에서는 목표 속도까지 가속하고, 입력을 놓으면 감속한다.
		const FVector TargetVelocity = InputDirection * Speed;
		const float Rate = bHasInput ? Accel : std::max(Deceleration, 0.0f);
		Velocity += (TargetVelocity - Velocity).GetClampedToMaxSize(Rate * DeltaTime);
		SteeringDirection = Velocity.GetSafeNormal();
	}
	else
	{
		const float OldSpeed = Velocity.Size();
		FVector Direction = Velocity.GetSafeNormal();
		if (SteeringDirection.SizeSquared() <= SMALL_NUMBER)
		{
			SteeringDirection = OldSpeed > SMALL_NUMBER ? Direction : InputDirection.GetSafeNormal();
		}
		if (bHasInput)
		{
			// 조향은 먼저 바뀐다. 실제 이동 방향은 아래의 접지력으로 뒤따라온다.
			const FVector Target = InputDirection.GetSafeNormal();
			const float Angle = std::atan2(SteeringDirection.X * Target.Y - SteeringDirection.Y * Target.X,
				SteeringDirection.Dot(Target));
			const float MaxTurn = FMath::DegreesToRadians(std::max(TurnRate, 0.0f)) * DeltaTime;
			const float Turn = std::clamp(Angle, -MaxTurn, MaxTurn);
			const float C = std::cos(Turn), S = std::sin(Turn);
			SteeringDirection = FVector(SteeringDirection.X * C - SteeringDirection.Y * S,
				SteeringDirection.X * S + SteeringDirection.Y * C, 0.0f);
		}
		if (OldSpeed <= SMALL_NUMBER)
		{
			Direction = SteeringDirection;
		}
		else
		{
			const float SlipAngle = std::atan2(Direction.X * SteeringDirection.Y - Direction.Y * SteeringDirection.X,
				Direction.Dot(SteeringDirection));
			const float GripAlpha = 1.0f - std::exp(-std::max(IceGrip, 0.0f) * DeltaTime);
			const float Turn = SlipAngle * GripAlpha;
			const float C = std::cos(Turn), S = std::sin(Turn);
			Direction = FVector(Direction.X * C - Direction.Y * S, Direction.X * S + Direction.Y * C, 0.0f);
		}
		// 빙판의 입력은 조향/가속이다. 역방향도 브레이크 대신 선회하므로 멈추지 않는다.
		const float NewSpeed = std::min(OldSpeed + Accel * InputDirection.Size() * DeltaTime, Speed);
		Velocity = Direction * NewSpeed;
	}
	UpdateComponentVelocity();

	// 가속 중 이동 거리는 프레임 시작/끝 속도의 평균으로 적분한다.
	const FVector Delta = (OldVelocity + Velocity) * (0.5f * DeltaTime);
	const float Distance = Delta.Size();
	if (Distance <= SMALL_NUMBER) return;

	FHitResult Hit(1.0f);
	const FVector Start = UpdatedComponent->GetWorldLocation();
	const bool bHit = GetOwner()->GetActorEnableCollision() && GetWorld()->SweepSingle(
		FRay(Start, Delta / Distance), Distance, std::max(CollisionRadius, 0.0f), Hit, GetOwner(), true);
	// SweepSingle은 이동 경로 전체를 검사하므로 빠른 이동도 얇은 벽에서 멈춘다.
	MoveUpdatedComponent(Delta * Hit.Time, UpdatedComponent->GetWorldRotation(), false, nullptr);
	if (bHit) HandleImpact(Hit, DeltaTime, Delta);
}

void USlidingMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	if (bDead || !Hit.bBlockingHit) return;
	bDead = true;
	InputDirection = FVector::ZeroVector;
	StopMovementImmediately();
	OnDeath.Broadcast(Hit.HitComponent ? Hit.HitComponent->GetOwner() : nullptr);
}

void USlidingMovementComponent::Respawn()
{
	Respawn(SpawnLocation);
}

void USlidingMovementComponent::Respawn(const FVector& Location)
{
	if (!UpdatedComponent) return;
	InputDirection = FVector::ZeroVector;
	SteeringDirection = FVector::ZeroVector;
	StopMovementImmediately();
	UpdatedComponent->SetWorldLocation(Location, false);
	// 이동 후 World::UpdateOverlaps가 진입/이탈 이벤트로 빙판 상태를 갱신한다.
	bDead = false;
}
