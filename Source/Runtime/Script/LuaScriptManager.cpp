#include "EnginePCH.h"
#include "LuaScriptManager.h"

#include "LuaBindings.h"
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

	State->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
		sol::lib::table, sol::lib::coroutine);

	State->set_function("print", [](sol::this_state S, sol::variadic_args Args)
		{
			sol::state_view Lua(S);
			sol::protected_function ToString = Lua["tostring"];

			FString Line;
			for (auto Arg : Args)
			{
				if (!Line.empty())
					Line += "\t";
				sol::protected_function_result Str = ToString(Arg);
				Line += Str.valid() ? Str.get<FString>() : FString("<?>");
			}
			HTR_LOG(Info, "[Lua] {}", Line);
		});

	FLuaBindings::Register(*State);
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
