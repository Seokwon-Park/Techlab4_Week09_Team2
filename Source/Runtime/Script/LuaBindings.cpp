#include "EnginePCH.h"
#include "LuaBindings.h"
#include <sol/sol.hpp>

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
}

namespace FLuaBindings
{
	void Register(sol::state& Lua)
	{
		// TODO(B): new_usertype<FVector>, FRotator, 액터 핸들(UUID), IsKeyDown, GetDeltaTime
		// TODO(B): wait, wait_until, wait_until_move_done, StartCoroutine
		RegisterMath(Lua);
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
