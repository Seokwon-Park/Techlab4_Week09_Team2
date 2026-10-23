#include "EnginePCH.h"
#include "LuaBindings.h"

#include <sol/sol.hpp>

#include "Components/MovementComponent.h"
#include "Core/EngineTimer.h"
#include "GameFramework/Actor.h"
#include "Input/InputSystem.h"
#include "Script/LuaActorRef.h"

namespace
{
	// FVector, FRotator
	void RegisterMath(sol::state& Lua)
	{
		Lua.new_usertype<FVector>("FVector",
			sol::call_constructor,
			sol::constructors<FVector(), FVector(float, float, float)>(),
			"x", &FVector::X,
			"y", &FVector::Y,
			"z", &FVector::Z,
			sol::meta_function::addition, [](const FVector& A, const FVector& B) { return A + B; },
			sol::meta_function::subtraction, [](const FVector& A, const FVector& B) { return A - B; },
			sol::meta_function::unary_minus, [](const FVector& A) { return -A; },

			sol::meta_function::multiplication, sol::overload(
				[](const FVector& V, float Scalar) { return V * Scalar; },
				[](const float Scalar, const FVector& V) { return V * Scalar; }
			),
			sol::meta_function::division, [](const FVector& V, float Scalar) { return V / Scalar; },
			sol::meta_function::equal_to, [](const FVector& A, const FVector& B) { return A == B; },
			sol::meta_function::to_string, [](const FVector& V)
			{ return std::format("({:.2f}, {:.2f}, {:.2f})", V.X, V.Y, V.Z); },

			"Length", &FVector::Length,
			"Normalize", [](const FVector& V) { return V.GetSafeNormal(); },
			"Dot", [](const FVector& A, const FVector& B) { return A.Dot(B); },
			"Cross", [](const FVector& A, const FVector& B) { return A.Cross(B); }
		);

		Lua.new_usertype<FRotator>("FRotator",
			sol::call_constructor,
			sol::constructors<FRotator(), FRotator(float, float, float)>(),
			"pitch", &FRotator::Pitch,
			"yaw", &FRotator::Yaw,
			"roll", &FRotator::Roll,

			sol::meta_function::to_string, [](const FRotator& R)
			{ return std::format("(P={:.1f}, Y={:.1f}, R={:.1f})", R.Pitch, R.Yaw, R.Roll); }
		);
	}

	UMovementComponent* FindFirstMovement(AActor& Actor)
	{
		for (UActorComponent* Comp : Actor.GetComponents())
		{
			if (UMovementComponent* MoveComp = Cast<UMovementComponent>(Comp))
			{
				return MoveComp;
			}
			return nullptr;
		}
	}

	void RegisterActor(sol::state& Lua)
	{
		Lua.new_usertype<FLuaActorRef>("Actor",
			sol::no_constructor, // 액터는 Lua에서 생성할 수 없음

			"UUID", sol::property([](const FLuaActorRef& Ref) { return Ref.UUID; }),
			"IsValid", [](const FLuaActorRef& Ref) { return Ref.Get() != nullptr; },
			"Name", sol::property([](const FLuaActorRef& Ref) { return Deref(Ref).GetName(); }),

			"Location", sol::property(
				// Getter
				[](const FLuaActorRef& Ref) { return Deref(Ref).GetActorLocation(); },
				// Setter
				[](const FLuaActorRef& Ref, const FVector& V)
				{
					if (USceneComponent* RootComp = Deref(Ref).GetRootComponent())
					{
						RootComp->SetWorldLocation(V, false);
					}
				}),
			"Rotation", sol::property(
				// Getter
				[](const FLuaActorRef& Ref) { return Deref(Ref).GetActorRotation(); },
				// Setter
				[](const FLuaActorRef& Ref, const FRotator& R)
				{
					if (USceneComponent* RootComp = Deref(Ref).GetRootComponent())
					{
						RootComp->SetWorldRotation(R, false);
					}
				}),
			// Velocity는 MovementComponent가 있는 경우에만 제공
			"Velocity", sol::property(
				// Getter
				[](const FLuaActorRef& Ref)
				{
					if (UMovementComponent* MoveComp = FindFirstMovement(Deref(Ref)))
					{
						return MoveComp->Velocity;
					}
					return FVector::ZeroVector;
				},
				// Setter
				[](const FLuaActorRef& Ref, const FVector& V)
				{
					if (UMovementComponent* MoveComp = FindFirstMovement(Deref(Ref)))
					{
						MoveComp->Velocity = V;
					}
					throw sol::error("Actor does not have a MovementComponent to set Velocity.");
				}),
			"PrintLocation", [](const FLuaActorRef& Ref)
			{
				const FVector Location = Deref(Ref).GetActorLocation();
				HTR_LOG(Info, "[Lua] {} ({:.2f}, {:.2f}, {:.2f})", Deref(Ref).GetName(), Location.X, Location.Y, Location.Z);
			},

			sol::meta_function::equal_to, [](const FLuaActorRef& A, const FLuaActorRef& B)
			{
				return A.UUID == B.UUID;
			},
			sol::meta_function::to_string, [](const FLuaActorRef& Ref)
			{
				return std::format("Actor#{}", Ref.UUID);
			}
		);
	}

	EKeyCode ToKeyCode(const std::string& Name)
	{
		if (Name.size() == 1 && std::isalnum(static_cast<unsigned char>(Name[0])))
		{
			// 알파벳 또는 숫자 키를 EKeyCode로 변환
			return static_cast<EKeyCode>(std::toupper(static_cast<unsigned char>(Name[0])));
		}

		static const TMap<std::string, EKeyCode> KeyMap =
		{
			{ "space", EKeyCode::Space },
			{ "enter", EKeyCode::Enter },
			{ "escape", EKeyCode::Escape },
			{ "tab", EKeyCode::Tab },
			{ "shift", EKeyCode::Shift },
			{ "backspace", EKeyCode::Backspace },
			{ "ctrl", EKeyCode::Control },
			{ "alt", EKeyCode::Alt },
			{ "left", EKeyCode::Left },
			{ "right", EKeyCode::Right },
			{ "up", EKeyCode::Up },
			{ "down", EKeyCode::Down },
		};

		std::string Lower = Name;
		std::transform(Lower.begin(), Lower.end(), Lower.begin(), [](unsigned char c) { return std::tolower(c); });
		if (const EKeyCode* Code = KeyMap.Find(Lower))
		{
			return *Code;
		}
		throw sol::error("Unknown key name: " + Name);
	}

	void RegisterInput(sol::state& Lua)
	{
		Lua.set_function("IsKeyDown", [](const std::string& K) {
			return FInputSystem::IsKeyDown(ToKeyCode(K));
			});
		Lua.set_function("IsKeyPressed", [](const std::string& K) {
			return FInputSystem::IsKeyPressed(ToKeyCode(K));
			});
		Lua.set_function("IsKeyReleased", [](const std::string& K) {
			return FInputSystem::IsKeyReleased(ToKeyCode(K));
			});

		// Tick(dt)의 dt를 쓰는 게 원칙. 코루틴처럼 dt를 못 받는 곳에서만 쓴다
		Lua.set_function("GetDeltaTime", []() {
			return EngineTimer::GetDeltaTime();
			});
		Lua.set_function("GetTime", []() {
			return EngineTimer::GetTotalTime();
			});
	}
}

namespace FLuaBindings
{
	void Register(sol::state& Lua)
	{
		// TODO(B): wait, wait_until, wait_until_move_done, StartCoroutine
		RegisterMath(Lua);
		RegisterActor(Lua);
		RegisterInput(Lua);
	}
}

FCoroutineScheduler& FCoroutineScheduler::Get()
{
	static FCoroutineScheduler Instance;
	return Instance;
}

void FCoroutineScheduler::Start(AActor* Owner, sol::function Func)
{
	// TODO(B): sol::thread 생성, 대기 목록에 추가 후 첫 resume
}

void FCoroutineScheduler::Tick(float DeltaTime)
{
	// TODO(B): 대기 조건이 풀린 코루틴 resume, 끝난 것은 제거
}

void FCoroutineScheduler::StopAll(AActor* Owner)
{
	// TODO(B)
}
