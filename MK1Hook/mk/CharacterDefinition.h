#pragma once
#include "..\utils.h"
#include "Engine.h"
#include "..\unreal\FName.h"
#include "..\unreal\TArray.h"

#define AI_DATA_OFFSET 280

struct CharacterContentDefinitionInfo {
	char pad[16];
	FName path;
	char _pad[16];
	FName skin;
	char __pad[528];

	void Set(FString name, int type = 7);

};
VALIDATE_SIZE(CharacterContentDefinitionInfo, 576);

struct Override
{
	int pad = -1;
	char _pad[12] = {};
	FName path;
	char __pad[16] = {};
};
VALIDATE_SIZE(Override, 40);

class CharacterDefinitionV2 {
public:
	char pad[0xD0];
	char _pad[16];
	FName path;
	char __pad[16];
	FName skin;

	void Set(CharacterContentDefinitionInfo* info);
	void SetPartner(CharacterContentDefinitionInfo* info);

	void SetAIDrone(int mko, int level);
	int  GetAIDroneLevel();
	int  GetAIDroneScript();
};

VALIDATE_OFFSET(CharacterDefinitionV2, path, 0xE0);
VALIDATE_OFFSET(CharacterDefinitionV2, skin, 0xF8);

class MainCharacter : public CharacterDefinitionV2 {
public:
	FName extraMoveset;
	TArray<Override> overrides;
};

VALIDATE_OFFSET(MainCharacter, extraMoveset, 0x100);
VALIDATE_OFFSET(MainCharacter, overrides, 0x0108);

class KameoCharacter : public CharacterDefinitionV2 {
public:
	TArray<Override> overrides;
};

VALIDATE_OFFSET(KameoCharacter, overrides, 0x0100);



