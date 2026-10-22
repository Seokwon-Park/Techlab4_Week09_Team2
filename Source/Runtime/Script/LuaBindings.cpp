#include "EnginePCH.h"
#include "LuaBindings.h"
#include <sol/sol.hpp>

namespace FLuaBindings
{
	void Register(sol::state& Lua)
	{
		// TODO(B): new_usertype<FVector>, FRotator, 액터 핸들(UUID), IsKeyDown, GetDeltaTime
		// TODO(B): wait, wait_until, wait_until_move_done, StartCoroutine
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
