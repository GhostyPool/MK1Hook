#pragma once
#include "..\unreal\UObject.h"
#include "..\utils.h"

#ifdef UpdateResource
#undef UpdateResource
#endif

#define LOCKSTATUS_UNLOCKED (uint8_t)0
#define LOCK_READ_ONLY (uint8_t)1
#define LOCK_READ_WRITE (uint8_t)2

struct FBulkDataBase
{
	char pad[16];
	int64 BulkDataSize;
	int64 BulkDataOffset;
	int BulkDataFlags;
	uint8_t LockStatus;
	char _pad[3];

	void* Realloc(int64 InElementCount)
	{
		static uintptr_t pat = _pattern(PATID_BulkData_Realloc);

		if (LockStatus == LOCK_READ_WRITE)
		{
			if (pat)
				return ((void* (__fastcall*)(FBulkDataBase*, int64))pat)(this, InElementCount);
			else
				return nullptr;
		}
		else
		{
			eLog::Message(__FUNCTION__, "BulkData isn't locked for read/write!");
			return nullptr;
		}
	}
	bool isSingleUse() const { return (BulkDataFlags & 0x8) != 0; }
	void* Lock(unsigned int LockFlags)
	{
		static uintptr_t pat = _pattern(PATID_BulkData_Lock);

		if (pat)
			return ((void* (__fastcall*)(FBulkDataBase*, unsigned int))pat)(this, LockFlags);
		else
			return nullptr;
	}
	void Unlock() const
	{
		static uintptr_t pat = _pattern(PATID_BulkData_Unlock);

		if (pat)
			((void(__fastcall*)(const FBulkDataBase*))pat)(this);
	}
};
VALIDATE_SIZE(FBulkDataBase, 40);
VALIDATE_OFFSET(FBulkDataBase, BulkDataFlags, 0x20);
VALIDATE_OFFSET(FBulkDataBase, LockStatus, 0x24);

struct FTexture2DMipMap
{
	int SizeX;
	int SizeY;
	int SizeZ;
	char _pad[4];
	FBulkDataBase BulkData;
};
VALIDATE_SIZE(FTexture2DMipMap, 56);
VALIDATE_OFFSET(FTexture2DMipMap, BulkData, 0x10);

struct FTexturePlatformData
{
	int SizeX;
	int SizeY;
	char _pad[4];
	int PixelFormat;
	char __pad[8];
	TArray<FTexture2DMipMap*> Mips;
	char ___pad[8];
};
VALIDATE_SIZE(FTexturePlatformData, 48);
VALIDATE_OFFSET(FTexturePlatformData, PixelFormat, 0x0C);
VALIDATE_OFFSET(FTexturePlatformData, Mips, 0x18);

class UTexture2D : public UObject
{
public:
	char _pad[360];
	FTexturePlatformData* PlatformData;
	char __pad[8];

	void UpdateResource()
	{
		static uintptr_t pat = _pattern(PATID_UTexture2D_UpdateResource);

		if (pat)
			((void(__fastcall*)(UTexture2D*))pat)(this);
	}
};
VALIDATE_SIZE(UTexture2D, 416);
VALIDATE_OFFSET(UTexture2D, PlatformData, 0x190);