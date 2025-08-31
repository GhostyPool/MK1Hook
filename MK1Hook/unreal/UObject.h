#pragma once
#include "FName.h"
#include <string>

class UClass;

class UObject {
public:
	void*		vtable;

	int			Flags;
	int			Index;
	UClass*		Class;
	FName		Name;
	UObject*	Outer;

	FName GetFName() const;
	FString GetName() const;
};


class UClass : public UObject {
public:

};
