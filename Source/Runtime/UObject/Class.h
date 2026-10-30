#pragma once

#include "Property.h"
#include "Function.h"
#include "Object.h"

using ClassConstructor = UObject * (*)(void* Memory);

class UClass : public UObject
{
public:
	UClass();
    ~UClass();

	FString Name;
	UClass* Super = nullptr;
	uint64 ClassSize = 0;
	ClassConstructor Constructor = nullptr;

	TArray<FProperty*> Properties; // 이 클래스에서 선언한 것만. UClass가 소유한다 (UE: UStruct::ChildProperties)
	TArray<FFunction> Functions;
	UObject* DefaultObject = nullptr;

	bool IsChildOf(const UClass* BaseClass) const;
	UObject* GetDefaultObject();

	inline const TArray<FProperty*>& GetProperties() const { return Properties; }
	inline const TArray<FFunction>& GetFunctions() const { return Functions; }

	const FFunction* FindFunction(const FString& InName) const;

	template<class T>
	T* GetDefaultObject()
	{
		return CastChecked<T>(GetDefaultObject());
	}

    // HHT가 만드는 코드: InClass->AddProperty<decltype(ThisClass::Speed)>("Speed", offsetof(ThisClass, Speed), CPF_Edit);
// 프로퍼티 클래스는 멤버 타입으로 고른다(TPropertyTypeFor). UPROPERTY(Type=LinearColor)면 HHT가 두 번째 인자로 직접 넘긴다.
// 만든 프로퍼티를 돌려주므로 HHT는 meta를 ->Meta = ...로 이어서 넣는다.
    template <typename T, typename TPropertyClass = typename TPropertyTypeFor<T>::Type>
    TPropertyClass* AddProperty(const FString& InName, uint32 InOffset, uint32 InFlags = CPF_Default)
    {
        if constexpr (std::is_void_v<TPropertyClass>)
        {
            static_assert(sizeof(T) == 0, "This type cannot be a UPROPERTY. See TPropertyTypeFor in Property.h.");
            return nullptr;
        }
        else
        {
            if constexpr (requires { typename TPropertyClass::TCppType; })
            {
                static_assert(sizeof(T) == sizeof(typename TPropertyClass::TCppType), "UPROPERTY Type= does not match the size of the member type.");
            }

            TPropertyClass* Property = new TPropertyClass();
            Property->Name = InName;
            Property->Owner = this;
            Property->Offset = InOffset;
            Property->PropertyFlags = InFlags;

            if constexpr (std::is_same_v<TPropertyClass, FObjectProperty>)
            {
                Property->PropertyClass = std::remove_pointer_t<T>::StaticClass();
            }
            else if constexpr (std::is_same_v<TPropertyClass, FEnumProperty>)
            {
                static_assert(TEnumInfo<T>::bReflected, "An enum used as a UPROPERTY needs UENUM().");
                if constexpr (TEnumInfo<T>::bReflected)
                {
                    // Get이 템플릿인 이유는 HHT가 만든 TEnumInfo의 주석 참고
                    Property->Enum = TEnumInfo<T>::template Get<>();
                }
                Property->ElementSize = sizeof(T);
            }

            Properties.Add(Property);
            return Property;
        }
    }

	template <auto Func>
	void AddFunction(const FString& InName, uint32 InFlags)
	{
		Functions.Add(TNativeFunction<Func>::Make(InName, InFlags));
	}
private:
	mutable int32 ClassUnique = 0;
	friend FName MakeUniqueObjectName(UObject*, const UClass*, FName);
};

//보류
inline void CopyProperties(UObject* Src, UObject* Dst, UClass* FromClass)
{
	for (UClass* c = FromClass; c; c = c->Super)
		for (const FProperty* p : c->Properties)
			memcpy((char*)Dst + p->Offset, (char*)Src + p->Offset, p->ElementSize);
}