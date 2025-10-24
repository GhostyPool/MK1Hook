#include "PluginInterface.h"
#include "..\gui\log.h"
#include "..\gui\imgui\imgui.h"

std::vector<PluginInfo> PluginInterface::plugins;

void PluginInterface::ScanFolder(wchar_t* path)
{
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	hFind = FindFirstFileW(path, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			PluginInfo plugin;
			ZeroMemory(&plugin, sizeof(PluginInfo));
			if (plugin.Load(ffd.cFileName))
				plugins.push_back(plugin);
		}
	} while (FindNextFileW(hFind, &ffd) != 0);
}

void PluginInterface::LoadPlugins()
{
	plugins.clear();
	wchar_t	moduleName[MAX_PATH] = {};

	GetModuleFileNameW(nullptr, moduleName, MAX_PATH);

	std::wstring str(moduleName);

	str = str.substr(0, str.find_last_of(L"/\\"));
	eLog::Message(__FUNCTION__, "Scanning %ws", str.c_str());
	str += L"\\*.ehp";


	ScanFolder((wchar_t*)str.c_str());

}

void PluginInterface::UnloadPlugins()
{
	for (unsigned int i = 0; i < plugins.size(); i++)
	{
		plugins[i].Unload();
	}
}

void PluginInterface::OnFrameTick()
{
	for (unsigned int i = 0; i < plugins.size(); i++)
	{
		if (plugins[i].pluginOnFrameTick)
			plugins[i].pluginOnFrameTick();
	}
}

void PluginInterface::OnFightStartup()
{
	for (unsigned int i = 0; i < plugins.size(); i++)
	{
		if (plugins[i].pluginOnFightStartup)
			plugins[i].pluginOnFightStartup();
	}
}

void PluginInterface::OnGameLogicJump(__int64 gameInfoPtr, char* mkoName, unsigned int functionHash)
{
	for (unsigned int i = 0; i < plugins.size(); i++)
	{
		if (plugins[i].pluginOnGameLogicJump)
			plugins[i].pluginOnGameLogicJump(gameInfoPtr, mkoName, functionHash);
	}
}

void PluginInterface::OnBeginPlay(__int64 actorPtr)
{
	for (unsigned int i = 0; i < plugins.size(); i++)
	{
		if (plugins[i].pluginOnBeginPlay)
			plugins[i].pluginOnBeginPlay(actorPtr);
	}
}

void PluginInterface::ProcessPluginTabs()
{
	if (plugins.size() > 0)
	{
		if (ImGui::BeginTabItem("Plugins"))
		{
			if (ImGui::BeginTabBar("##plugintabs"))
			{
				for (unsigned int i = 0; i < plugins.size(); i++)
				{	
					if (plugins[i].pluginTabFunction)
					{
						static char labelName[128] = {};
						sprintf_s(labelName, "%s##p%d", plugins[i].GetPluginTabName(), i);
						if (ImGui::BeginTabItem(labelName))
						{
							plugins[i].pluginTabFunction();
							ImGui::EndTabItem();
						}
					}

				}
				
				ImGui::EndTabBar();
			}
			ImGui::EndTabItem();
		}
	}

}

int PluginInterface::GetNumPlugins()
{
	return (int)plugins.size();
}


const char* PluginInfo::GetPluginName()
{
	return szPluginName;
}

const char* PluginInfo::GetPluginTabName()
{
	return szPluginTabName;
}

const char* PluginInfo::GetPluginProject()
{
	return szPluginProject;
}

bool PluginInfo::Load(wchar_t* path)
{
	hModule = LoadLibraryW(path);

	if (hModule)
	{
		pluginGetPluginName = (const char*(*)())GetProcAddress(hModule, "GetPluginName");
		if (!pluginGetPluginName)
		{
			eLog::Message(__FUNCTION__, "ERROR: Could not find GetPluginName for %ws!", path);
			return false;
		}
		pluginGetPluginProject = (const char* (*)())GetProcAddress(hModule, "GetPluginProject");
		if (!pluginGetPluginProject)
		{
			eLog::Message(__FUNCTION__, "ERROR: Could not find GetPluginProject for %ws!", path);
			return false;
		}
		pluginGetPluginTabName = (const char* (*)())GetProcAddress(hModule, "GetPluginTabName");
		if (!pluginGetPluginTabName)
		{
			eLog::Message(__FUNCTION__, "ERROR: Could not find GetPluginTabName for %ws!", path);
			return false;
		}
		pluginOnFrameTick = (void(*)())GetProcAddress(hModule, "OnFrameTick");
		pluginOnFightStartup = (void(*)())GetProcAddress(hModule, "OnFightStartup");
		pluginOnGameLogicJump = (void(*)(__int64, char*, unsigned int))GetProcAddress(hModule, "OnGameLogicJump");
		pluginOnBeginPlay = (void(*)(__int64))GetProcAddress(hModule, "OnBeginPlay");
		pluginOnInitialize = (void(*)())GetProcAddress(hModule, "OnInitialize");
		if (!pluginOnInitialize)
		{
			eLog::Message(__FUNCTION__, "ERROR: Could not find OnInitialize for %ws!", path);
			return false;
		}
		pluginOnShutdown = (void(*)())GetProcAddress(hModule, "OnShutdown");
		if (!pluginOnShutdown)
		{
			eLog::Message(__FUNCTION__, "ERROR: Could not find OnShutdown for %ws!", path);
			return false;
		}
		pluginTabFunction = (void(*)())GetProcAddress(hModule, "TabFunction");

		sprintf_s(szPluginName, pluginGetPluginName());
		sprintf_s(szPluginProject, pluginGetPluginProject());
		sprintf_s(szPluginTabName, pluginGetPluginTabName());


		if (!(strcmp(szPluginProject, PROJECT_NAME) == 0))
		{
			eLog::Message(__FUNCTION__, "ERROR: Plugin %ws is not built for this project!", path);
			return false;
		}

		eLog::Message(__FUNCTION__, "INFO: Loaded %s (%ws)!", szPluginName, path);

		pluginOnInitialize();

		return true;
	}

	return false;
}

void PluginInfo::Unload()
{
	if (pluginOnShutdown)
		pluginOnShutdown();

	if (hModule)
	{
		FreeLibrary(hModule);
		hModule = 0;
	}
}
