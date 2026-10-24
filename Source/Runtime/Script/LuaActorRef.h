#pragma once

#include "Core/Types.h"

class AActor;
class UWorld;

struct FLuaActorRef
{
	UWorld* World = nullptr;
	AActor* Ptr = nullptr;
	uint32 UUID = 0;

	static FLuaActorRef Make(AActor* Actor);

	// 살아 있으면 액터, 파괴됐으면 nullptr
	AActor* Get() const;
};

AActor& Deref(const FLuaActorRef& Ref);
