#pragma once
#include "..\unreal\FName.h"
#include "..\unreal\UTexture2D.h"
#include "..\unreal\FWeakObjectPtr.h"
#include "..\utils.h"
#include "../gui/imgui/imgui.h"
#include <string>
#include <array>
#include <unordered_map>
#include <cstdio>

extern void(__fastcall* orgSetTextureParameterValue)(int64, FName, UTexture2D*);

struct FNameHash
{
	size_t operator()(const FName& name) const noexcept
	{
		return (static_cast<size_t>(name.Index) << 32) ^ static_cast<size_t>(name.Number);
	}
};
struct PaletteData
{
	FWeakObjectPtr				weakPtr;
	const std::string			name;
	std::array<ImVec4, 16>		colours;
	bool						appliedPalette;
	bool						inMenu;

	PaletteData(const UTexture2D* texObj)
		: weakPtr(texObj),
		name([w = texObj->GetName().GetStr()] { return std::string(w, w + wcslen(w)); }()),
		colours{},
		appliedPalette(false),
		inMenu(false)
	{
		colours.fill(ImVec4(1, 1, 1, 1));
	}

	void ApplyPaletteColour();
};
extern std::unordered_map<FName, PaletteData, FNameHash> g_palettes;

struct PaletteUI
{
	std::string				name;
	std::array<ImVec4, 16>	colours;
	bool					appliedPalette;
	FName					fname;

	PaletteUI(const PaletteData& data, const FName& fname)
		: name(data.name),
		colours(data.colours),
		appliedPalette(data.appliedPalette),
		fname(fname)
	{
	}

	static void CheckPalettes();

	bool OpenPaletteLoadDialog();
	void OpenPaletteSaveDialog() const;
};

void SetPaletteTexture_Hook(int64 ptr, FName ParameterName, UTexture2D* Value);