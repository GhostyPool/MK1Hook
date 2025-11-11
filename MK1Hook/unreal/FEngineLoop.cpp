#include "FEngineLoop.h"
#include "..\plugin\Hooks.h"
#include <utility>

void FEngineLoop::Tick()
{
	static uintptr_t pat = _pattern(PATID_FEngineLoop_Tick);
	if (pat)
	{
		((void(__fastcall*)(FEngineLoop*))pat)(this);

		ProcessTickQueue();

		PluginDispatch();
	}

}

void FEngineLoop::ProcessTickQueue()
{
	std::queue<std::function<void()>> local_queue;

	{
		std::lock_guard<std::mutex> lock(tick_queue_mutex);
		if (!tick_queue.empty())
		{
			std::swap(local_queue, tick_queue);
		}
	}

	while (!local_queue.empty())
	{
		(local_queue.front())();
		local_queue.pop();
	}
}