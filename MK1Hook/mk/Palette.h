#pragma once
#include "..\unreal\FName.h"
#include "..\unreal\UTexture2D.h"
#include "..\unreal\FWeakObjectPtr.h"
#include "..\utils.h"
#include "../gui/imgui/imgui.h"
#include <string>
#include <array>
#include <cstdio>

extern void(__fastcall* orgSetTextureParameterValue)(int64, FName, UTexture2D*);

struct PaletteData
{
	FWeakObjectPtr				weakTex;
	const std::string			texName;
	std::array<ImVec4, 16>		colours{};
	bool						inMenu = false;
	bool						changedColour = false;

	PaletteData(const UTexture2D* texObj)
		: weakTex(texObj),
		texName([&] {
			char temp[256] = {};
			snprintf(temp, sizeof(temp), "%ws", texObj->GetName().GetStr());
			return std::string(temp);
		}())
	{
		colours.fill(ImVec4(1, 1, 1, 1));
	}
};

void SetPaletteTexture_Hook(int64 ptr, FName ParameterName, UTexture2D* Value);
void ApplyPaletteColour(PaletteData* data);