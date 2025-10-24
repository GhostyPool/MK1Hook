#include "AActor.h"
#include "..\plugin\PluginInterface.h"

void(*orgBeginPlay)(__int64) = nullptr;

void AActor_BeginPlay(__int64 a1)
{
	PluginInterface::OnBeginPlay(a1);

	if (orgBeginPlay)
		orgBeginPlay(a1);
}