#include "Palette.h"
#include "../unreal/UObject.h"
#include "../plugin/Menu.h"
#include "../plugin/Settings.h"
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <shobjidl.h>
#include <combaseapi.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#ifdef UpdateResource
#undef UpdateResource
#endif

void(__fastcall* orgSetTextureParameterValue)(int64, FName, UTexture2D*) = nullptr;

static const char presetHeader[8] = "Palette";

struct FNameHash
{
	size_t operator()(const FName& name) const noexcept
	{
		return (static_cast<size_t>(name.Index) << 32) ^ static_cast<size_t>(name.Number);
	}
};

static std::unordered_map<FName, PaletteData, FNameHash> g_palettes;

static bool WritePresetToDisk(const std::array<ImVec4, 16>& colours, const wchar_t* path)
{
	FILE* f = _wfopen(path, L"wb");
	if (!f)
		return false;

	if (std::fwrite(presetHeader, sizeof(presetHeader), 1, f) != 1)
	{
		std::fclose(f);
		return false;
	}

	for (const ImVec4& col : colours)
	{
		const float colFloats[4] = { col.x, col.y, col.z, col.w };
		if (std::fwrite(colFloats, sizeof(float), 4, f) != 4)
		{
			std::fclose(f);
			return false;
		}
	}

	std::fclose(f);
	return true;
}

static bool ReadPresetFromDisk(std::array<ImVec4, 16>& colours, const wchar_t* path)
{
	FILE* f = _wfopen(path, L"rb");
	if (!f)
		return false;

	char header[sizeof(presetHeader)] = {};
	if (std::fread(header, sizeof(presetHeader), 1, f) != 1 || memcmp(header, presetHeader, sizeof(presetHeader)) != 0)
	{
		std::fclose(f);
		return false;
	}

	for (ImVec4& col : colours)
	{
		float newCol[4] = {};
		if (std::fread(newCol, sizeof(float), 4, f) != 4)
		{
			std::fclose(f);
			return false;
		}
		col = ImVec4(newCol[0], newCol[1], newCol[2], newCol[3]);
	}

	std::fclose(f);
	return true;
}

static void UninitializeCom(bool needsUninit)
{
	if (needsUninit)
		CoUninitialize();
}

static void ShowMessageBox(bool success, bool saving)
{
	if (success)
	{
		const wchar_t* type = saving ? L"saved" : L"loaded";
		std::wstring message = std::wstring(L"Palette ") + type + L" successfully.";
		MessageBoxW(nullptr, message.c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		const wchar_t* type = saving ? L"save" : L"load";
		std::wstring message = std::wstring(L"Failed to ") + type + L" palette!";
		MessageBoxW(nullptr, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
	}
}

static void ExitWithError(HRESULT hr, bool needsUninit, bool saving)
{
	UninitializeCom(needsUninit);
	ShowMessageBox(false, saving);
	const char* func = saving ? "OpenPaletteSaveDialog" : "OpenPaletteLoadDialog";
	eLog::Message(func, "Failed to save palette, error: 0x%08X", (unsigned)hr);
}

bool OpenPaletteLoadDialog(std::array<ImVec4, 16>& colours)
{
	bool needUninit = false;
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (hr == S_OK) needUninit = true;
	else if (hr == S_FALSE) needUninit = true;
	else if (hr == RPC_E_CHANGED_MODE) needUninit = false;
	else
	{
		ExitWithError(hr, false, false);
		return false;
	}

	IFileOpenDialog* dialog = nullptr;
	hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (FAILED(hr))
	{
		ExitWithError(hr, needUninit, false);
		return false;
	}

	COMDLG_FILTERSPEC filters[] =
	{
		{ L"Palette preset (*.palette)", L"*.palette" },
		{ L"All Files (*.*)", L"*.*" }
	};
	dialog->SetFileTypes(ARRAYSIZE(filters), filters);
	dialog->SetFileTypeIndex(1);
	dialog->SetTitle(L"Load palette preset");

	hr = dialog->Show(nullptr);
	if (FAILED(hr))
	{
		dialog->Release();
		UninitializeCom(needUninit);
		if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return false;
		ExitWithError(hr, false, false);
		return false;
	}

	IShellItem* file = nullptr;
	hr = dialog->GetResult(&file);
	if (FAILED(hr) || !file)
	{
		dialog->Release();
		ExitWithError(hr, needUninit, false);
		return false;
	}

	PWSTR pszPath = nullptr;
	hr = file->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
	file->Release();
	if (FAILED(hr) || !pszPath)
	{
		dialog->Release();
		ExitWithError(hr, needUninit, false);
		return false;
	}

	bool loaded = ReadPresetFromDisk(colours, pszPath);
	CoTaskMemFree(pszPath);
	dialog->Release();
	UninitializeCom(needUninit);
	ShowMessageBox(loaded, false);
	return loaded;
}

void OpenPaletteSaveDialog(const std::array<ImVec4, 16>& colours, const wchar_t* fileName)
{
	bool needUninit = false;
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (hr == S_OK) needUninit = true;
	else if (hr == S_FALSE) needUninit = true;
	else if (hr == RPC_E_CHANGED_MODE) needUninit = false;
	else
	{
		ExitWithError(hr, false, true);
		return;
	}

	IFileSaveDialog* dialog = nullptr;
	hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (FAILED(hr))
	{
		ExitWithError(hr, needUninit, true);
		return;
	}

	COMDLG_FILTERSPEC filters[] =
	{
		{ L"Palette preset (*.palette)", L"*.palette" },
		{ L"All Files (*.*)", L"*.*" }
	};
	dialog->SetFileTypes(ARRAYSIZE(filters), filters);
	dialog->SetFileTypeIndex(1);
	dialog->SetDefaultExtension(L"palette");
	dialog->SetFileName(fileName);
	dialog->SetTitle(L"Save palette preset");

	hr = dialog->Show(nullptr);
	if (FAILED(hr))
	{
		dialog->Release();
		UninitializeCom(needUninit);
		if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return;
		ExitWithError(hr, false, true);
		return;
	}

	IShellItem* file = nullptr;
	hr = dialog->GetResult(&file);
	if (FAILED(hr) || !file)
	{
		dialog->Release();
		ExitWithError(hr, needUninit, true);
		return;
	}

	PWSTR pszPath = nullptr;
	hr = file->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
	file->Release();
	if (FAILED(hr) || !pszPath)
	{
		dialog->Release();
		ExitWithError(hr, needUninit, true);
		return;
	}

	bool saved = WritePresetToDisk(colours, pszPath);
	CoTaskMemFree(pszPath);
	dialog->Release();
	UninitializeCom(needUninit);
	ShowMessageBox(saved, true);
}

static wchar_t palettesFolder[MAX_PATH] = {};
static bool gotPalettesFolder = false;
static bool cantLoadFromDisk = false;

static void CheckAndLoadFromDisk(PaletteData& data)
{
	if (!gotPalettesFolder)
	{
		GetModuleFileNameW(nullptr, palettesFolder, MAX_PATH);
		PathRemoveFileSpecW(palettesFolder);

		bool success = false;
		int folderLen = MultiByteToWideChar(CP_UTF8, 0, SettingsMgr->strPalettesFolder.c_str(), -1, nullptr, 0);
		if (folderLen > 1)
		{
			std::wstring folder;
			folder.resize(folderLen - 1);
			if (MultiByteToWideChar(CP_UTF8, 0, SettingsMgr->strPalettesFolder.c_str(), -1, folder.data(), folderLen))
			{
				if (PathAppendW(palettesFolder, folder.c_str()))
				{
					success = true;
				}
			}
		}

		if (!success)
		{
			eLog::Message(__FUNCTION__, "An error occurred when trying to use the specified palettes folder: %s! Falling back to default folder \"Palettes\"!", SettingsMgr->strPalettesFolder.c_str());
			if (!PathAppendW(palettesFolder, L"Palettes"))
			{
				eLog::Message(__FUNCTION__, "Palettes folder path exceeded maximum allowed path length!");
				cantLoadFromDisk = true;
				return;
			}
		}
		gotPalettesFolder = true;
	}

	wchar_t filePath[MAX_PATH] = {};
	wcsncpy_s(filePath, palettesFolder, _TRUNCATE);

	std::wstring fileName(data.texName.begin(), data.texName.end());
	fileName += L".palette";

	if (!PathAppendW(filePath, fileName.c_str()))
	{
		eLog::Message(__FUNCTION__, "Could not proceed trying to load: %s from disk, path exceeded maximum allowed path length!", data.texName.c_str());
		cantLoadFromDisk = true;
		return;
	}

	DWORD attr = GetFileAttributesW(filePath);
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) return;

	bool loaded = ReadPresetFromDisk(data.colours, filePath);
	if (loaded)
	{
		eLog::Message("MK1Hook::Info()", "Loaded palette: %s from disk.", data.texName.c_str());
		ApplyPaletteColour(&data);
		data.appliedPalette = true;
	}
	else
		eLog::Message(__FUNCTION__, "Could not load palette: %s from disk, its format is invalid!", data.texName.c_str());
}

static bool IsSupported(UTexture2D* texture)
{
	if (texture->PlatformData->PixelFormat == 0x2)
		return true;
	else
		return false;
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
			eLog::Message(__FUNCTION__, "Was unable to apply new palette colour since BulkData returned nullptr!");
			return;
		}

		texture->UpdateResource();
	}
	else
		eLog::Message(__FUNCTION__, "Cannot apply new palette colour to invalid palette: %s!", data->texName.c_str());
}

void SetPaletteTexture_Hook(int64 ptr, FName ParameterName, UTexture2D* Value)
{
	FName palName = Value->GetFName();
	auto i = g_palettes.find(palName);
	if (i != g_palettes.end())
	{
		PaletteData& existingPal = i->second;

		if (!existingPal.weakTex.IsValid())
		{
			existingPal.weakTex = Value;

			if (existingPal.appliedPalette)
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
		if (IsSupported(Value))
		{
			auto result = g_palettes.emplace(palName, PaletteData(Value));
			PaletteData& data = result.first->second;

			if (!cantLoadFromDisk)
				CheckAndLoadFromDisk(data);

			TheMenu->m_Palettes.emplace_back(&data);
			data.inMenu = true;
		}
	}

	if (orgSetTextureParameterValue)
		orgSetTextureParameterValue(ptr, ParameterName, Value);
}