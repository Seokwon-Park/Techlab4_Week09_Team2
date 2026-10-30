#include "EnginePCH.h"
#include "UObject/Property.h"

#include "UObject/Class.h"
#include "Asset/AssetManager.h" //??? 의존성 제거 필요

// ---- FColorProperty ----

void FColorProperty::ExportJson(json& Out, const void* Value) const
{
    Out = GetPropertyValue(Value).DWColor();
}

void FColorProperty::ImportJson(const json& In, void* Value) const
{
    GetPropertyValue(Value).DWColor() = In.get<uint32>();
}

void FColorProperty::SerializeItem(FStructuredArchive::FSlot Slot, void* Value) const
{
    Slot << GetPropertyValue(Value);
}

// ---- FLinearColorProperty ----

void FLinearColorProperty::ExportJson(json& Out, const void* Value) const
{
    const FLinearColor& Color = GetPropertyValue(Value);
    Out = json::array({ Color.R, Color.G, Color.B, Color.A });
}

void FLinearColorProperty::ImportJson(const json& In, void* Value) const
{
    if (!In.is_array() || In.size() != 4)
    {
        HTR_LOG(Warning, "Load: '{}' is not an [R, G, B, A] array, keeping default", Name);
        return;
    }
    FLinearColor& Color = GetPropertyValue(Value);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Color.RGBA[Index] = In[Index].get<float>();
    }
}

void FLinearColorProperty::SerializeItem(FStructuredArchive::FSlot Slot, void* Value) const
{
    Slot << GetPropertyValue(Value);
}

// ---- FObjectProperty ----

void FObjectProperty::ExportJson(json& Out, const void* Value) const
{
    // 경로가 없는 에셋(런타임에 만든 인스턴스 등)은 되찾을 방법이 없으니 null로 쓴다
    URenderAsset* Asset = Cast<URenderAsset>(GetPropertyValue(Value));
    if (Asset && !Asset->GetPath().empty())
        Out = Asset->GetPath();
    else
        Out = nullptr;
}

void FObjectProperty::ImportJson(const json& In, void* Value) const
{
    // null로 저장된 건 "되찾을 수 없는 값"이었으므로 기본값을 유지한다
    if (In.is_null())
    {
        return;
    }

    const FString AssetPath = In.get<FString>();
    URenderAsset* Asset = nullptr;

    if (PropertyClass && PropertyClass->IsChildOf(UStaticMesh::StaticClass()))
    {
        Asset = UAssetManager::GetAssetByPath<UStaticMesh>(AssetPath);
    }
    else
    {
        Asset = UAssetManager::GetAssetByPath<URenderAsset>(AssetPath);
    }

    // 못 찾으면 생성자가 넣어둔 기본값을 그대로 둔다.
    // (파일 하나가 없어져도 씬은 열리고, 액터가 사라지지 않고 기본 모양으로 보인다)
    if (!Asset || (PropertyClass && !Asset->IsA(PropertyClass)))
    {
        HTR_LOG(Warning, "Load: asset '{}' not found for {}, keeping default", AssetPath, Name);
        return;
    }

    GetPropertyValue(Value) = Asset;
}

void FObjectProperty::SerializeItem(FStructuredArchive::FSlot Slot, void* Value) const
{
    UObject*& Ref = GetPropertyValue(Value);
    UObject* Previous = Ref;
    Slot << Ref;

    // 불러온 객체가 프로퍼티가 기대하는 클래스가 아니면 되돌린다
    if (Slot.GetUnderlyingArchive().IsLoading() && Ref && PropertyClass && !Ref->IsA(PropertyClass))
    {
        HTR_LOG(Warning, "Load: '{}' is not a {}, keeping default", Name, PropertyClass->Name);
        Ref = Previous;
    }
}

// ---- FEnumProperty ----

void FEnumProperty::ExportJson(json& Out, const void* Value) const
{
    // UENUM은 이름으로 저장한다. enum 항목 순서를 바꿔도 씬 파일이 깨지지 않는다.
    if (Enum)
    {
        Out = Enum->ToName(GetValue(Value));
    }
}

void FEnumProperty::ImportJson(const json& In, void* Value) const
{
    if (!Enum)
    {
        return;
    }
    int64 Loaded = 0;
    if (In.is_number_integer())
        SetValue(Value, In.get<int64>());
    else if (In.is_string() && Enum->FromName(In.get<std::string>(), Loaded))
        SetValue(Value, Loaded);
    else
        HTR_LOG(Warning, "Load: '{}' has no enum entry {}, keeping default", Name, In.dump());
}

void FEnumProperty::SerializeItem(FStructuredArchive::FSlot Slot, void* Value) const
{
    // 이름으로 저장한다 (json 경로와 같은 규칙)
    FString EntryName = Enum ? Enum->ToName(GetValue(Value)) : FString();
    Slot << EntryName;

    int64 Loaded = 0;
    if (Slot.GetUnderlyingArchive().IsLoading() && Enum)
    {
        if (Enum->FromName(EntryName, Loaded))
            SetValue(Value, Loaded);
        else
            HTR_LOG(Warning, "Load: '{}' has no enum entry '{}', keeping default", Name, EntryName);
    }
}