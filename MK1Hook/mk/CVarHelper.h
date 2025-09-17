#pragma once

class CVarSetter
{
public:
	static void SetCVarByName(const wchar_t* name, unsigned int value)
	{
		static uintptr_t pat = _pattern(PATID_SetCVarByName);

		if (pat)
			((__int64(__fastcall*)(const wchar_t*, unsigned int, __int64))pat)(name, value, 0);
	}
};