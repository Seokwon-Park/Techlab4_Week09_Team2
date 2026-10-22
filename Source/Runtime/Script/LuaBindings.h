#pragma once

#include <sol/forward.hpp>

class AActor;

// 엔진 API를 Lua에 노출한다. 빌드 시간 때문에 바인딩은 LuaBindings.cpp 한 파일에 모은다. 담당: B, 소비자: A
namespace FLuaBindings
{
	// FVector, FRotator, AActor(안전 핸들), Input, Time
	void Register(sol::state& Lua);
}

// wait(sec), wait_until(cond) 등으로 멈춘 코루틴을 들고 있다가 조건이 되면 재개한다. 담당: B
class FCoroutineScheduler
{
public:
	static FCoroutineScheduler& Get();

	void Start(AActor* Owner, sol::function Func);
	void Tick(float DeltaTime);     // A가 월드 Tick에서 1회 호출
	void StopAll(AActor* Owner);    // A가 액터 EndPlay·Destroy에서 호출
};
