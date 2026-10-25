#include "EnginePCH.h"
#include "LuaBindings.h"

#include <sol/sol.hpp>

#include "Components/MovementComponent.h"
#include "Core/EngineTimer.h"
#include "GameFramework/Actor.h"
#include "Input/InputSystem.h"
#include "Script/LuaActorRef.h"
#include "Script/CoroutineScheduler.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

namespace
{
	TArray<FLuaActorRef> PendingDestroies;

	AActor* OwnerFromEnv(sol::this_environment& ThisEnv)
	{
		if (!ThisEnv)
			return nullptr;

		sol::environment& Env = ThisEnv;
		sol::optional<FLuaActorRef> Ref = Env["obj"];
		return Ref
			? Ref->Get()
			: nullptr;
	}

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
		}
		return nullptr;
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
					else
					{
						throw sol::error("Actor does not have a MovementComponent to set Velocity.");
					}
				}),
			"PrintLocation", [](const FLuaActorRef& Ref)
			{
				const FVector Location = Deref(Ref).GetActorLocation();
				HTR_LOG(Info, "[Lua] {} ({:.2f}, {:.2f}, {:.2f})", Deref(Ref).GetName(), Location.X, Location.Y, Location.Z);
			},

			"Destroy", [](const FLuaActorRef& Ref)
			{
				// 이미 파괴되지 않은 액터만 PendingDestroies에 추가
				if (Ref.Get())
				{
					PendingDestroies.Add(Ref);
				}
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

		Lua.set_function("IsLeftMouseButtonPressed", []() {
			return FInputSystem::IsMousePressed(EMouseButton::Left);
			});

		// Viewport 포지션을 반환한다.
		Lua.set_function("GetMousePosition", [](sol::this_environment ThisEnv) {
			AActor* Owner = OwnerFromEnv(ThisEnv);
			if (!Owner)
			{
				throw sol::error("GetMousePosition must be called from a coroutine with an Actor context.");
			}

			UWorld* World = Owner->GetWorld();
			if (!World)
			{
				throw sol::error("GetMousePosition must be called from a coroutine with an Actor context.");
			}

			FWorldContext* Context =
				GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;

			if (!Context || !Context->GameViewport)
			{
				throw sol::error("GetMousePosition must be called from a coroutine with an Actor context.");
			}

			FGameViewportClient* Viewport = Context->GameViewport.get();

			const FRect& ViewportRect = Viewport->GetRect();
			
			//const float MouseX = FInputSystem::GetMouseX();
			//const float MouseY = FInputSystem::GetMouseY();
			POINT Mouse{};
			if (!::GetCursorPos(&Mouse))
			{
				throw sol::error("Failed to get mouse cursor position.");
			}

			const float LocalX = static_cast<float>(Mouse.x) - ViewportRect.X;
			const float LocalY = static_cast<float>(Mouse.y) - ViewportRect.Y;

			return std::make_tuple(
				LocalX,
				LocalY);
			});

		Lua.set_function("ScreenToWorldRay",
			[](sol::this_environment Env, float X, float Y)
			-> std::tuple<sol::optional<FVector>, sol::optional<FVector>>
			{
				AActor* Owner = OwnerFromEnv(Env);
				UWorld* World = Owner ? Owner->GetWorld() : nullptr;

				if (!GEngine || !World)
				{
					return {};
				}

				FWorldContext* Context =
					GEngine->GetWorldContextFromWorld(World);

				if (!Context || !Context->GameViewport)
				{
					return {};
				}

				FRay Ray;
				if (!Context->GameViewport->ScreenToWorldRay(X, Y, &Ray))
				{
					return {};
				}

				return {
					sol::optional<FVector>(Ray.Origin),
					sol::optional<FVector>(Ray.Direction)
				};
			}
		);

		// Tick(dt)의 dt를 쓰는 게 원칙. 코루틴처럼 dt를 못 받는 곳에서만 쓴다
		Lua.set_function("GetDeltaTime", []() {
			return EngineTimer::GetDeltaTime();
			});
		Lua.set_function("GetTime", []() {
			return EngineTimer::GetTotalTime();
			});
	}

	void RegisterCoroutine(sol::state& Lua)
	{
		Lua.set_function("StartCoroutine",
			[](sol::this_environment ThisEnv, sol::function Fn)
			{
				FCoroutineScheduler::Get().Start(OwnerFromEnv(ThisEnv), Fn);
			});

		// C++ 함수가 반환값을 yield 값으로 남기고 멈춘다. 코루틴 밖에서 부르면 Lua 에러
		Lua.set_function("wait", sol::yielding(
			[](float Seconds)
			{
				return std::make_tuple("time", Seconds);
			}));
		Lua.set_function("wait_frame", sol::yielding(
			[]()
			{
				return;
			}));
		Lua.set_function("wait_until", sol::yielding(
			[](sol::function Cond)
			{
				return std::make_tuple("until", Cond);
			}));
		Lua.set_function("wait_until_move_done", sol::yielding(
			[]()
			{
				return "move";
			}));

		Lua.set_function("move_to",
			[](sol::this_environment ThisEnv, const FVector& Target, float Speed)
			{
				if (AActor* Owner = OwnerFromEnv(ThisEnv))
				{
					FCoroutineScheduler::Get().MoveTo(Owner, Target, Speed);
				}
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
		RegisterCoroutine(Lua);
	}

	void FlushPendingDestroy(UWorld* World)
	{
		TArray<FLuaActorRef> ToDestroy = std::move(PendingDestroies);
		PendingDestroies = TArray<FLuaActorRef>();

		for (const FLuaActorRef& Ref : ToDestroy)
		{
			if (Ref.World != World)
			{
				continue;
			}
			if (AActor* Actor = Ref.Get()) // 살아 있는 액터만 Destroy
			{
				Actor->Destroy();
			}
		}
	}

	void ReleaseCallbacks(AActor* Owner)
	{
		// TODO
	}
}