#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class ETokenType
{
	Identifier, 
	Symbol,
	Number,
	String,
	Char
};

struct FToken
{
	ETokenType Type = ETokenType::Symbol;
	std::string Text;
	int Line = 0;

	// 식별자·기호를 글자 그대로 비교한다. 문자열 안의 "UCLASS"는 걸리지 않는다.
	bool Is(const char* InText) const
	{
		return (Type == ETokenType::Identifier || Type == ETokenType::Symbol) && Text == InText;
	}
};

// UCLASS(...) / UPROPERTY(...) 괄호 안의 항목 하나. Key, Key=Value, Key=(하위 목록)
struct FSpecifier
{
	std::string Key;
	std::string Value;
	std::vector<FSpecifier> Children;
	int Line = 0;
};

// meta 키 하나와 생성 코드에 들어갈 float 리터럴. 예: { "ClampMin", "0.0f" }
struct FMetaValue
{
	std::string Key;
	std::string Literal
};