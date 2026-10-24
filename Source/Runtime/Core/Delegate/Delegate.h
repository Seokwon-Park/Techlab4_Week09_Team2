#include <functional>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "Containers/Array.h"
#include "UObject/Object.h"

template<typename... Args>
//기본 Delegate 타입과 선언 매크로
class TDelegate
{
public:
	using HandlerType = std::function<void(Args...)>;

	TDelegate() = default;
	TDelegate(const TDelegate&) = delete; // 복사생성 금지
	TDelegate& operator=(const TDelegate&) = delete; // 복사생성 금지

	// 일반 함수나 람다 등록
	void Add(const HandlerType& handler)
	{
		AddInternal(std::move(handler), nullptr, 0);
	}

	// UObject를 상속받는 클래스의 멤버 함수 바인딩
	template<typename T>
	void AddDynamic(T* Instance, void (T::* Func)(Args...))
	{
		if (!Instance || !Func) return;

		const UObject* Object = Instance;
		uint32 UUID = 0;

		//살아있는 객체목록에 등록된 객체만 바인딩
		if (!GetObjectUUID(Object, UUID)) return;

		AddInternal(
			[Instance, Func](Args... args)
			{
				(Instance->*Func)(args...);
			},
			Object,
			UUID
		);
	}

	void Broadcast(Args... args)
	{
		auto Snapshot = Handlers;

		for (auto& Entry : Snapshot)
		{
			// RemoveALL 또는 Clear로 해제되었다면 실행하지 않음
			if (!Entry->bActive) continue;

			if (Entry->Instance)
			{
				uint32 CurrentUUID = 0;

				if (!GetObjectUUID(Entry->Instance, CurrentUUID)
					|| CurrentUUID != Entry->UUID)
				{
					Entry->bActive = false;
					continue;
				}
			}

			Entry->Handler(args...);
		}

		RemoveInactive();

	}

	void Clear()
	{
		for (auto& Entry : Handlers)
		{
			Entry->bActive = false;
		}
		Handlers.clear();
	}

	// 해당 객체가 AddDynamic으로 등록한 함수를 모두 제거
	void RemoveAll()
	{
		if (!Instance) return;

		for (auto& Entry : Handlers)
		{
			if (Entry->Instance == Instance)
			{
				Entry->bActive = false;
			}
		}

		RemoveInactive();
	}

private:
	struct FHandler
	{
		HandlerType Handler; // 어느 객체의 함수인지

		const UObject* Instance = nullptr; // 어느 객체의 함수인지
		uint32 UUID = 0;

		bool bActive = true; // 활성화 상태인지
	};

	void AddInternal(
		HandlerType handler, 
		const UObject* Instance, 
		uint32 UUID) 
	{
		if (!handler) return;

		auto Entry = std::make_shared<FHandler>();

		Entry->Handler = std::move(handler);
		Entry->Instance = Instance;
		Entry->UUID = UUID;

		Handlers.push_back(std::move(Entry));
	}

	// Instance를 직접 역참조하지 않고,
	// 현재 살아 있는 객체 목록에서 일치하는 객체를 찾는다.
	static bool GetObjectUUID(
		const UObject* Instance,
		uint32& OutUUID)
	{
		if (!Instance) return false;

		for (const UObject* Object : GUObjectArray)
		{
			if (Object && Object == Instance)
			{
				OutUUID = Object->GetUUID();
				return true;
			}
		}

		return false;
	}

	// 비활성화된 구독을 실제 배열에서 제거
	void RemoveInactive()
	{
		Handlers.erase(
			std::remove_if(
				Handlers.begin(),
				Handlers.end(),
				[](const std::shared_ptr<FHandler>& Entry)
				{
					return !Entry->bActive;
				}
			),
			Handlers.end()
		);
	}

	std::vector<std::shared_ptr<FHandler>> Handlers;
};

#define DECLARE_DELEGATE(Name, ...) TDelegate<__VA_ARGS__> Name
