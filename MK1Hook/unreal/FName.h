#pragma once
#include "FString.h"
#include "..\utils.h"

enum EFindName
{
	FNAME_Find,
	FNAME_Add,
	FNAME_Replace,
};


class FName {
public:
	int Index;
	int Number;

	FString* ToString(FString* str) const;

	FName();
	FName(const char* Name, EFindName FindType, int formal);
	FName(const wchar_t* Name, EFindName FindType, int formal);

	bool operator==(const FName& Other) const { return Index == Other.Index && Number == Other.Number; }
};
