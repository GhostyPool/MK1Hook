#pragma once

namespace EgsAPI
{
	bool Initialize();
	bool IsItemOwned(const char* itemId);
	bool IsApiInitialized();
	bool IsOwnershipQueried();
	const char* GetUserId();
}