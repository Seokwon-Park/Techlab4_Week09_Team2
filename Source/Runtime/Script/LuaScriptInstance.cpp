#include "EnginePCH.h"
#include "LuaScriptInstance.h"
#include "LuaScriptManager.h"
#include <sol/sol.hpp>

struct FLuaScriptInstance::FImpl
{
	sol::environment Env;
	fs::file_time_type LastWriteTime;
};

FLuaScriptInstance::FLuaScriptInstance()
	: Impl(std::make_unique<FImpl>())
{
}

FLuaScriptInstance::~FLuaScriptInstance() = default;

bool FLuaScriptInstance::Load(AActor* InOwner, const FString& InPath)
{
	Owner = InOwner;
	Path = InPath;
	// TODO(A): Env = sol::environment(State, sol::create, State.globals()); Env["obj"] = Owner;
	//          State.safe_script_file(Path, Env) 결과 확인, LastWriteTime 기록
	return false;
}

void FLuaScriptInstance::ReloadIfChanged()
{
	// TODO(A): fs::last_write_time 비교 후 Load(Owner, Path)
}

void FLuaScriptInstance::Call(const char* Name)
{
	// TODO(A): sol::protected_function F = Impl->Env[Name]; if (F.valid()) { 결과 검사 }
}

void FLuaScriptInstance::Call(const char* Name, float Value)
{
	// TODO(A)
}

void FLuaScriptInstance::Call(const char* Name, AActor* Other)
{
	// TODO(A): Other는 B의 안전 핸들로 감싸서 넘긴다
}
