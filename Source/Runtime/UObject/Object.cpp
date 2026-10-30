#include "EnginePCH.h"
#include "Object.h"

#include "Core/EngineStatics.h"
#include "UObject/Class.h"

#include "Serialization/TypeSerializer.h"
#include "Serialization/MathSerialization.h"
#include "Serialization/Formatter/BinaryArchiveFormatter.h"



TArray<UObject*> GUObjectArray;

UObject::UObject()
{
	ObjectUUID = FEngineStatics::GetUUID();
	InternalIndex = GUObjectArray.Num();
	GUObjectArray.Add(this);
}

UObject::UObject(bool bRegister)
{
	bIsRegistered = bRegister;
}

UObject::~UObject()
{
	if (bIsRegistered)
	{
		if (ClassPrivate)
		{
			UnhashObject(this, ClassPrivate);
		}
		int32 LastIndex = GUObjectArray.Num() - 1;
		if (InternalIndex != LastIndex)
		{
			UObject* MovedObject = GUObjectArray[LastIndex];
			GUObjectArray[InternalIndex] = MovedObject;
			MovedObject->InternalIndex = InternalIndex; // 옮겨간 오브젝트 인덱스 갱신
		}
		GUObjectArray.RemoveLast();
	}
}

UClass* UObject::StaticClass()
{
	static UClass c;
	static bool bIsInit = false;
	if (!bIsInit)
	{
		c.Name = "Object";
		c.Super = nullptr;
		c.ClassSize = sizeof(UObject);
		c.Constructor = [](void* Memory) -> UObject*
			{
				return ::new (Memory) UObject();
			};
		bIsInit = true;
	}
	return &c;
}

bool UObject::IsA(const UClass* Class) const
{
	const UClass* ThisClass = GetClass();
	return ThisClass && ThisClass->IsChildOf(Class);
}

void UObject::Serialize(json& Handle, bool bIsLoading)
{
	// 타입별 처리는 각 FProperty 서브클래스가 한다 (Property.h)
	for (UClass* c = GetClass(); c; c = c->Super)
	{
		for (const FProperty* Property : c->GetProperties())
		{
			if (Property->HasAnyFlags(CPF_Transient))
				continue;

			if (bIsLoading && !Handle.contains(Property->Name))
				continue;

			void* Value = Property->ContainerPtrToValuePtr(this);
			if (bIsLoading)
				Property->ImportJson(Handle[Property->Name], Value);
			else
				Property->ExportJson(Handle[Property->Name], Value);
		}
	}
}

void UObject::Serialize(FArchive& Ar)
{
	FBinaryArchiveFormatter Formatter(Ar);
	FStructuredArchive StructuredAr(Formatter);
	Serialize(StructuredAr.Open().EnterRecord());
	StructuredAr.Close();
}

void UObject::Serialize(FStructuredArchive::FRecord Record)
{
	SerializeScriptProperties(Record.EnterField("Properties"));
}

void UObject::SerializeScriptProperties(FStructuredArchive::FSlot Slot)
{
	FStructuredArchive::FRecord Properties = Slot.EnterRecord();

	for (UClass* c = GetClass(); c; c = c->Super)
	{
		for (const FProperty* Property : c->GetProperties())
		{
			TOptional<FStructuredArchive::FSlot> Slot = Properties.TryEnterField(Property->Name.c_str(), true);
			if (!Slot.IsSet()) continue;

			Property->SerializeItem(Slot.GetValue(), Property->ContainerPtrToValuePtr(this));
		}
	}
}



