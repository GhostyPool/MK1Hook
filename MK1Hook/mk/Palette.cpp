#include "Palette.h"
#include "../unreal/UObject.h"
#include "../plugin/Menu.h"
#include "../plugin/Settings.h"
#include <unordered_set>
#include <array>
#include <ranges>
#include <shobjidl.h>
#include <combaseapi.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#ifdef UpdateResource
#undef UpdateResource
#endif

void(__fastcall* orgSetTextureParameterValue)(int64, FName, UTexture2D*) = nullptr;

std::unordered_map<FName, PaletteData, FNameHash> g_palettes;
std::queue<Pal_event> pal_event_queue;
std::mutex pal_event_queue_mtx;

static const char presetHeader[8] = "Palette";
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
static void PrintErrorMessage(HRESULT hr, bool saving)
{
	const char* func = saving ? "OpenPaletteSaveDialog" : "OpenPaletteLoadDialog";
	eLog::Message(func, "Error: 0x%08X", (unsigned)hr);
}

bool OpenPaletteLoadDialog(std::array<ImVec4, 16>& colours)
{
	bool loaded = false;
	bool needUninit = false;
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (hr == S_OK) needUninit = true;
	else if (hr == S_FALSE) needUninit = true;
	else if (hr == RPC_E_CHANGED_MODE) needUninit = false;
	else
	{
		PrintErrorMessage(hr, false);
		goto leave;
	}

	IFileOpenDialog* dialog = nullptr;
	hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (FAILED(hr))
	{
		PrintErrorMessage(hr, false);
		goto leave;
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
	if (FAILED(hr)) goto release_dialog;

	IShellItem* file = nullptr;
	hr = dialog->GetResult(&file);
	if (FAILED(hr) || !file) goto release_dialog;

	PWSTR pszPath = nullptr;
	hr = file->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
	file->Release();
	if (FAILED(hr) || !pszPath) goto release_dialog;

	loaded = ReadPresetFromDisk(colours, pszPath);
	CoTaskMemFree(pszPath);

release_dialog:
	dialog->Release();

leave:
	if (needUninit) CoUninitialize();
	if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)) ShowMessageBox(loaded, false);
	return loaded;
}
void OpenPaletteSaveDialog(const std::array<ImVec4, 16>& colours, const wchar_t* fileName)
{
	bool saved = false;
	bool needUninit = false;
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (hr == S_OK) needUninit = true;
	else if (hr == S_FALSE) needUninit = true;
	else if (hr == RPC_E_CHANGED_MODE) needUninit = false;
	else
	{
		PrintErrorMessage(hr, true);
		goto leave;
	}

	IFileSaveDialog* dialog = nullptr;
	hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (FAILED(hr))
	{
		PrintErrorMessage(hr, true);
		goto leave;
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
	if (FAILED(hr)) goto release_dialog;

	IShellItem* file = nullptr;
	hr = dialog->GetResult(&file);
	if (FAILED(hr) || !file) goto release_dialog;

	PWSTR pszPath = nullptr;
	hr = file->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
	file->Release();
	if (FAILED(hr) || !pszPath) goto release_dialog;

	saved = WritePresetToDisk(colours, pszPath);
	CoTaskMemFree(pszPath);

release_dialog:
	dialog->Release();

leave:
	if (needUninit) CoUninitialize();
	if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)) ShowMessageBox(saved, true);
}

static wchar_t palettesFolder[MAX_PATH] = {};
static bool gotPalettesFolder = false;
static void CheckAndLoadFromDisk(PaletteData& data)
{
	if (!gotPalettesFolder)
	{
		GetModuleFileNameW(nullptr, palettesFolder, MAX_PATH);
		PathRemoveFileSpecW(palettesFolder);

		std::wstring folder(SettingsMgr->szPalettesFolder, SettingsMgr->szPalettesFolder + 255);
		if (PathAppendW(palettesFolder, folder.c_str()))
			goto success;

		eLog::Message(__FUNCTION__, "An error occurred when trying to use the specified palettes folder: %s! Falling back to default folder \"Palettes\"!", SettingsMgr->szPalettesFolder);
		if (!PathAppendW(palettesFolder, L"Palettes"))
		{
			eLog::Message(__FUNCTION__, "Could not proceed trying to load palettes from disk, path exceeded maximum allowed path length!");
			return;
		}

	success:
		gotPalettesFolder = true;
	}

	wchar_t filePath[MAX_PATH] = {};
	wcsncpy_s(filePath, palettesFolder, _TRUNCATE);

	std::wstring fileName(data.name.begin(), data.name.end());
	fileName += L".palette";

	if (!PathAppendW(filePath, fileName.c_str()))
	{
		eLog::Message(__FUNCTION__, "Could not proceed trying to load: %s from disk, path exceeded maximum allowed path length!", data.name.c_str());
		return;
	}

	DWORD attr = GetFileAttributesW(filePath);
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) return;

	bool loaded = ReadPresetFromDisk(data.colours, filePath);
	if (loaded)
	{
		eLog::Message("MK1Hook::Info()", "Loaded palette: %s from disk.", data.name.c_str());
		ApplyPaletteColour(&data);
		data.appliedPalette = true;
	}
	else
		eLog::Message(__FUNCTION__, "Could not load palette : %s from disk, its format is invalid!", data.name.c_str());
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
	if (data->weakPtr.IsValid())
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

		UTexture2D* texture = static_cast<UTexture2D*>(data->weakPtr.Get());
		FBulkDataBase& bulkdata = texture->PlatformData->Mips.Get(0)->BulkData;

		if (bulkdata.isSingleUse())
			bulkdata.BulkDataFlags &= ~0x8;

		void* data = bulkdata.Lock(LOCK_READ_WRITE);
		if (!data) data = bulkdata.Realloc(2048);

		if (!data)
		{
			eLog::Message(__FUNCTION__, "Unable to apply new palette colour as BulkData is still nullptr after reallocation!");
			return;
		}

		memcpy(data, bytes.data(), 2048);

		bulkdata.Unlock();

		texture->UpdateResource();
	}
	else
		eLog::Message(__FUNCTION__, "Cannot apply new palette colour to invalid palette: %s!", data->name.c_str());
}

void SetPaletteTexture_Hook(int64 ptr, FName ParameterName, UTexture2D* Value)
{
	const FName& palName = Value->GetFName();
	auto i = g_palettes.find(palName);
	if (i != g_palettes.end())
	{
		PaletteData& existingPal = i->second;

		if (!existingPal.weakPtr.IsValid())
		{
			existingPal.weakPtr = Value;

			if (existingPal.appliedPalette)
				ApplyPaletteColour(&existingPal);

			if (!existingPal.inMenu)
			{
				{
					std::unique_lock<std::shared_mutex> lock(TheMenu->m_pal_ui_mtx);
					TheMenu->m_Palettes_UI.emplace_back(existingPal, i->first);
				}
				existingPal.inMenu = true;
			}
		}
	}
	else
	{
		if (Value->PlatformData->PixelFormat == 0x2)
		{
			auto entry = g_palettes.emplace(palName, PaletteData(Value)).first;
			PaletteData& data = entry->second;

			if (SettingsMgr->bLoadPalettesAtStartup)
				CheckAndLoadFromDisk(data);

			{
				std::unique_lock<std::shared_mutex> lock(TheMenu->m_pal_ui_mtx);
				TheMenu->m_Palettes_UI.emplace_back(data, entry->first);
			}
			data.inMenu = true;
		}
	}

	if (orgSetTextureParameterValue)
		orgSetTextureParameterValue(ptr, ParameterName, Value);
}

void CheckPalettes_Tick()
{
	std::vector<size_t> invalid_index;
	{
		std::shared_lock<std::shared_mutex> lock(TheMenu->m_pal_ui_mtx);
		for (size_t i = 0; i < TheMenu->m_Palettes_UI.size(); i++)
		{
			if (!TheMenu->m_Palettes_UI[i].weakPtr->IsValid())
				invalid_index.push_back(i);
		}
	}

	if (!invalid_index.empty())
	{
		std::unique_lock<std::shared_mutex> lock(TheMenu->m_pal_ui_mtx);
		for (size_t i : invalid_index | std::views::reverse)
		{
			PaletteData& palData = g_palettes.at(*TheMenu->m_Palettes_UI[i].fname);
			palData.colours = TheMenu->m_Palettes_UI[i].colours;
			palData.inMenu = false;
			TheMenu->m_Palettes_UI.erase(TheMenu->m_Palettes_UI.begin() + i);
		}
	}

	{
		std::lock_guard<std::mutex> lock(pal_event_queue_mtx);
		while (!pal_event_queue.empty())
		{
			Pal_event& event = pal_event_queue.front();
			PaletteData& palData = g_palettes.at(*event.payload.fname);
			switch (event.type)
			{
			case Pal_event_type::Apply:
				palData.colours = event.payload.colours;
				ApplyPaletteColour(&palData);
				palData.appliedPalette = true;
				break;
			case Pal_event_type::Reset:
				palData.appliedPalette = false;
				break;
			}
			pal_event_queue.pop();
		}
	}
}