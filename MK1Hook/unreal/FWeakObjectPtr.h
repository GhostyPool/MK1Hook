#pragma once
#include "UObject.h"
#include "..\utils.h"

struct FWeakObjectPtr
{
public:
	int ObjectIndex;
	int ObjectSerialNumber;

	FWeakObjectPtr()
	{
		ObjectIndex = -1;
		ObjectSerialNumber = 0;
	}

	FWeakObjectPtr(const UObject* Object)
	{
		(*this) = Object;
	}

	void operator=(const UObject* Object)
	{
		static uintptr_t pat = _pattern(PATID_FWeakObjectPtr_Equal);

		if (pat)
			((void (__fastcall*)(FWeakObjectPtr*, const UObject*))pat)(this, Object);
	}

	FWeakObjectPtr& operator=(const FWeakObjectPtr& Other) = default;

	UObject* Get() const
	{
		static uintptr_t pat = _pattern(PATID_FWeakObjectPtr_Get);

		if (pat)
			return ((UObject * (__fastcall*)(const FWeakObjectPtr*))pat)(this);
		else
			return nullptr;
	}

	bool IsValid() const
	{
		static uintptr_t pat = _pattern(PATID_FWeakObjectPtr_IsValid);

		if (pat)
			return ((bool(__fastcall*)(const FWeakObjectPtr*))pat)(this);
		else
			return false;
	}
};