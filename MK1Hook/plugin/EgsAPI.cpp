#include "EgsAPI.h"
#include "..\gui\log.h"
#include "EgsInternal.h"
#include <unordered_map>

static bool bEgsAPIOk = false;
static bool bEgsAPIOwnershipQueried = false;
static std::unordered_map<std::string, bool> g_itemOwnerships;

static const char* szItemIds[] =
{
	"33daec9ee0604218ab599352fc3b557b", //Definitive edition
	"8a7d295d3304407bb29a1804fe344827", //Definitive edition upgrade

	"5fdaa8674fb846cebecce1567e9a4dbb", //KP2
	"bdb30fd4e307414dacc92ee6c70497e5", //Noob Saibot
	"e44ff76a565349cea54e2811669f2da0", //Cyrax
	"3a56692519ce460391ca290cd7a0060c", //Sektor
	"e9eec22e8be345a28c1bddfc63c18dc5", //Ghostface
	"6be3fa19847a4d169f122a83071472da", //Conan
	"71dd666c8a75409ebf9c1252157219a7", //T1000

	"983a48cc30704b72bd5672c63f833004", //KP1
	"721fd20fe9c24d799c2b2b3f9fa0e626", //Quan chi
	"1c257910730c4df8aa9bc51082c41978", //Ermac
	"535194755ce3437f9dc848f55406f7ce", //Peacemaker
	"c5e71125a9c14f56b6e40b674717f72f", //Omni-man
	"c43c98f6c32d4948b20cfbe31b93a788", //Homelander
	"8a002e7a6ca74ecc9a0835305207ba78", //Takeda
	
	"3913e447017c4582afa247fc0a1e5b6d", //Khameleon
	"89d7a322d1bb4e3fbd6f10c506e0e59b", //Ferra
	"febf5faafb164d5d87140f0ad3d8f458", //Mavado
	"c3c79cd9f23144b88c5bd632ed83cbc0", //Madam Bo
	"ad1f61074db44890bc3265a9c7859c3a", //Janet Cage
	"a78b139fd6d546ae94abc11398768d7b", //Tremor
	
	"b0948456eb5f457d9c660c75832c8564" //Shang Tsung
};

bool EgsAPI::Initialize()
{
	eLog::Message(__FUNCTION__, "Init");
	
	if (EgsInternal::bEgsInternalOk)
	{
		EOS_Ecom_QueryOwnershipOptions options{
			.ApiVersion = EOS_ECOM_QUERYOWNERSHIP_API_LATEST,
			.LocalUserId = EgsInternal::pEpicAccountId,
			.CatalogItemIds = szItemIds,
			.CatalogItemIdCount = sizeof(szItemIds) / sizeof(szItemIds[0]),
			.CatalogNamespace = nullptr
		};

		auto OnQueryOwnershipCallback = [](const EOS_Ecom_QueryOwnershipCallbackInfo* Data)
			{
				if (Data->ResultCode == EOS_EResult::EOS_Success)
				{
					const EOS_Ecom_ItemOwnership* items = Data->ItemOwnership;
					for (unsigned int i = 0; i < Data->ItemOwnershipCount; ++i)
					{
						bool isOwned = false;
						if (items[i].OwnershipStatus == EOS_EOwnershipStatus::EOS_OS_Owned)
							isOwned = true;

						g_itemOwnerships.emplace(items[i].Id, isOwned);
					}
					bEgsAPIOwnershipQueried = true;
				}
			};

		EgsInternal::QueryOwnership(&options, OnQueryOwnershipCallback);

		eLog::Message(__FUNCTION__, "INFO: Init OK");
		bEgsAPIOk = true;
		return true;
	}
	eLog::Message(__FUNCTION__, "INFO: Init FAILED");
	return false;
}

bool EgsAPI::IsItemOwned(const char* itemId)
{
	if (bEgsAPIOwnershipQueried)
	{
		auto i = g_itemOwnerships.find(std::string(itemId));
		if (i != g_itemOwnerships.end())
			return i->second;
	}
	eLog::Message(__FUNCTION__, "ERROR: Ownership not yet queried!");
	return false;
}
bool EgsAPI::IsApiInitialized() { return bEgsAPIOk; }
bool EgsAPI::IsOwnershipQueried() { return bEgsAPIOwnershipQueried; }
const char* EgsAPI::GetUserId()
{
	if (EgsInternal::szUserId)
		return EgsInternal::szUserId;

	return nullptr;
}