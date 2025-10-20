#include "FEngineLoop.h"
#include "..\plugin\Hooks.h"
#include "..\mk\Palette.h"

void FEngineLoop::Tick()
{
	static uintptr_t pat = _pattern(PATID_FEngineLoop_Tick);
	if (pat)
	{
		((void(__fastcall*)(FEngineLoop*))pat)(this);

		CheckPalettes_Tick();

		PluginDispatch();
	}

}
