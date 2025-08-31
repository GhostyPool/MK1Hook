#include "Palette.h"
#include "../unreal/UObject.h"
#include "../plugin/Menu.h"
#include <unordered_map>
#include <unordered_set>
#include <array>

#ifdef UpdateResource
#undef UpdateResource
#endif

void(__fastcall* orgSetTextureParameterValue)(int64, FName, UTexture2D*) = nullptr;

struct FNameHash
{
	size_t operator()(const FName& name) const noexcept
	{
		return (static_cast<size_t>(name.Index) << 32) ^ static_cast<size_t>(name.Number);
	}
};

static std::unordered_map<FName, PaletteData, FNameHash> g_palettes;

static std::unordered_set<FName, FNameHash> g_unsupported_palettes;

static bool IsSupported(UTexture2D* texture)
{
	if (texture->PlatformData->PixelFormat == 0x2)
		return true;
	else
	{
		if (g_unsupported_palettes.insert(texture->GetFName()).second)
		{
			static char name[256] = {};
			snprintf(name, sizeof(name), "%ws", texture->GetName().GetStr());
			eLog::Message("MK1Hook::Info()", "Palette texture: %s is unsupported!", name);
		}
		
		return false;
	}
}

static std::array<uint8_t, 4> RGBAToBGRA(ImVec4 colour)
{
	uint8_t R = colour.x * 255.f;
	uint8_t G = colour.y * 255.f;
	uint8_t B = colour.z * 255.f;
	uint8_t A = colour.w * 255.f;

	return { B, G, R, A };
}

void ApplyPaletteColour(PaletteData* data)
{
	if (data->weakTex.IsValid())
	{
		std::array<uint8_t, 2048> bytes{};

		//16 colour segments
		for (int i = 0; i < 16; i++)
		{
			std::array<uint8_t, 4> pixel = RGBAToBGRA(data->colours[i]);
			uint8_t* segment = bytes.data() + i * (32 * 4);

			//32 pixels each
			for (int p = 0; p < 32; p++) {
				memcpy(segment + p * 4, pixel.data(), 4);
			}
		}

		UTexture2D* texture = static_cast<UTexture2D*>(data->weakTex.Get());
		FBulkDataBase& bulkdata = texture->PlatformData->Mips.Get(0)->BulkData;

		if (bulkdata.isSingleUse())
			bulkdata.BulkDataFlags &= ~0x8;

		void* data = bulkdata.Lock(LOCK_READ_WRITE);

		if (!data)
		{
			data = bulkdata.Realloc(2048);
		}

		bool success = false;
		if (data)
		{
			memcpy(data, bytes.data(), 2048);
			success = true;
		}

		bulkdata.Unlock();

		if (!success)
		{
			eLog::Message("MK1Hook::Info()", "Was unable to apply new palette colour since BulkData returned nullptr!");
			return;
		}

		texture->UpdateResource();
	}
	else
		eLog::Message("MK1Hook::Info()", "Cannot apply new palette colour to invalid palette: %s!", data->texName.c_str());
}

void SetPaletteTexture_Hook(int64 ptr, FName ParameterName, UTexture2D* Value)
{
	if (IsSupported(Value)) {
		FName palName = Value->GetFName();
		auto i = g_palettes.find(palName);
		if (i != g_palettes.end())
		{
			PaletteData& existingPal = i->second;

			if (!existingPal.weakTex.IsValid())
			{
				existingPal.weakTex = Value;

				if (existingPal.changedColour)
				{
					ApplyPaletteColour(&existingPal);
				}

				if (!existingPal.inMenu)
				{
					TheMenu->m_Palettes.emplace_back(&existingPal);
					existingPal.inMenu = true;
				}
			}
		}
		else
		{
			auto result = g_palettes.emplace(palName, PaletteData(Value));
			PaletteData& data = result.first->second;

			TheMenu->m_Palettes.emplace_back(&data);
			data.inMenu = true;
		}
	}

	if (orgSetTextureParameterValue)
		orgSetTextureParameterValue(ptr, ParameterName, Value);
}