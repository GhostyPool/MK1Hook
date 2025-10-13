#pragma once
#include "..\unreal\FName.h"
#include "..\unreal\UTexture2D.h"
#include "..\unreal\FWeakObjectPtr.h"
#include "..\utils.h"
#include "../gui/imgui/imgui.h"
#include <string>
#include <array>
#include <variant>
#include <mutex>
#include <queue>
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
	std::array<ImVec4, 16>		colours{};
	bool						appliedPalette = false;
	bool						inMenu = false;

	PaletteData(const UTexture2D* texObj)
		: weakPtr(texObj),
		name([w = texObj->GetName().GetStr()] { return std::string(w, w + wcslen(w)); }())
	{
		colours.fill(ImVec4(1, 1, 1, 1));
	}
};
extern std::unordered_map<FName, PaletteData, FNameHash> g_palettes;

struct PaletteUI
{
	std::string				name;
	std::array<ImVec4, 16>	colours;
	bool					appliedPalette = false;

	const FName* fname;
	const FWeakObjectPtr* weakPtr;

	PaletteUI(PaletteData& data, const FName& fname)
		: name(data.name),
		colours(data.colours),
		appliedPalette(data.appliedPalette),
		fname(&fname),
		weakPtr(&data.weakPtr)
	{
	}
};

struct Pal_event_payload
{
	const FName* fname;
	const std::array<ImVec4, 16>	colours{};

	Pal_event_payload(const FName* fname)
		: fname(fname)
	{
	}

	Pal_event_payload(const FName* fname, const std::array<ImVec4, 16>& colours)
		: fname(fname),
		colours(colours)
	{
	}
};
enum class Pal_event_type { Apply, Reset };
struct Pal_event
{
	const Pal_event_payload payload;
	const Pal_event_type type;

	Pal_event(const Pal_event_payload& payload, const Pal_event_type& type)
		: payload(payload),
		type(type)
	{
	}
};
extern std::queue<Pal_event> pal_event_queue;
extern std::mutex pal_event_queue_mtx;

//Saving/loading
bool OpenPaletteLoadDialog(std::array<ImVec4, 16>& colours);
void OpenPaletteSaveDialog(const std::array<ImVec4, 16>& colours, const wchar_t* fileName);
void ApplyPaletteColour(PaletteData* data);

void SetPaletteTexture_Hook(int64 ptr, FName ParameterName, UTexture2D* Value);