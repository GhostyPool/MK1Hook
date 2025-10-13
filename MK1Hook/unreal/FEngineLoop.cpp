#include "FEngineLoop.h"
#include "..\plugin\Hooks.h"
#include "..\plugin\Menu.h"
#include "..\mk\Palette.h"
#include <ranges>

void FEngineLoop::Tick()
{
	static uintptr_t pat = _pattern(PATID_FEngineLoop_Tick);
	if (pat)
	{
		((void(__fastcall*)(FEngineLoop*))pat)(this);

		//Palette validity check
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

		PluginDispatch();
	}

}
