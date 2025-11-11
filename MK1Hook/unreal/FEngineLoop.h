#pragma once
#include "..\utils.h"
#include <mutex>
#include <queue>
#include <functional>

class FEngineLoop {
public:
	void Tick();

	inline static std::queue<std::function<void()>> tick_queue;
	inline static std::mutex tick_queue_mutex;

private:
	static void ProcessTickQueue();
};