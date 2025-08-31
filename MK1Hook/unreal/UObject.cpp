#include "UObject.h"

FName UObject::GetFName() const { return Name; }

FString UObject::GetName() const
{
	FString name;
	GetFName().ToString(&name);

	return name;
}