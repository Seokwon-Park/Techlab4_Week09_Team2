#pragma once

#include <functional>

// 멀티캐스트 델리게이트. 담당: C, 소비자: 전원
// UE와 달리 단일/멀티를 나누지 않고 전부 멀티캐스트로 둔다.
using FDelegateHandle = uint64;

template<typename... Args>
class TDelegate
{
public:
	using FHandler = std::function<void(Args...)>;

	FDelegateHandle Add(FHandler Handler)
	{
		// TODO(C): 핸들러 저장, 증가하는 핸들 반환
		return 0;
	}

	// Obj가 파괴되면 Broadcast 때 건너뛰고 제거한다
	template<typename T>
	FDelegateHandle AddDynamic(T* Obj, void (T::*Func)(Args...))
	{
		// TODO(C): Obj의 생존 확인(UUID 등)을 같이 저장
		return 0;
	}

	void Remove(FDelegateHandle Handle)
	{
		// TODO(C)
	}

	void Broadcast(Args... InArgs)
	{
		// TODO(C): Broadcast 도중 Remove가 불려도 안전하게 복사본을 순회
	}

	void Clear()
	{
		// TODO(C)
	}

	bool IsBound() const { return false; }
};

// 시그니처를 이름으로 고정하는 매크로. 예) DECLARE_DELEGATE(FOnHealthChanged, float);
#define DECLARE_DELEGATE(DelegateName, ...) using DelegateName = TDelegate<__VA_ARGS__>
