#include "EnginePCH.h"
#include "LuaScriptManager.h"
#include <sol/sol.hpp>

FLuaScriptManager& FLuaScriptManager::Get()
{
	static FLuaScriptManager Instance;
	return Instance;
}

FLuaScriptManager::~FLuaScriptManager()
{
	Shutdown();
}

void FLuaScriptManager::Initialize()
{
	if (State)
		return;

	State = new sol::state();
	// TODO(A): open_libraries(base, math, string, table, coroutine), print 재정의, FLuaBindings::Register(*State)
}

void FLuaScriptManager::Shutdown()
{
	delete State;
	State = nullptr;
}

sol::state& FLuaScriptManager::GetState()
{
	if (!State)
		Initialize();
	return *State;
}

FString FLuaScriptManager::MakeScriptPath(const FString& SceneName, const FString& ActorName)
{
	return "Scripts/" + SceneName + "_" + ActorName + ".lua";
}

FString FLuaScriptManager::GetTemplatePath()
{
	return "Scripts/template.lua";
}

bool FLuaScriptManager::CreateScriptFromTemplate(const FString& Path)
{
	// TODO(A): fs::copy_file(GetTemplatePath(), Path)
	return false;
}

bool FLuaScriptManager::OpenScriptInEditor(const FString& Path)
{
	// TODO(A): ShellExecuteW(nullptr, L"open", ...) 반환값 확인
	return false;
}
