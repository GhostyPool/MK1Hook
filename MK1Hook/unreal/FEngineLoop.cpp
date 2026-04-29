#include "FEngineLoop.h"
#include "..\plugin\Hooks.h"
#include <utility>

void FEngineLoop::Tick()
{
	static uintptr_t pat = _pattern(PATID_FEngineLoop_Tick);
	if (pat)
	{
		((void(__fastcall*)(FEngineLoop*))pat)(this);

		PluginDispatch();
	}

}