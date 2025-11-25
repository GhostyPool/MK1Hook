#include "EgsInternal.h"
#include "..\gui\log.h"
#include "..\minhook\include\MinHook.h"
#include "..\epic\eos_init.h"
#include "..\epic\eos_sdk.h"
#include "..\epic\eos_ecom.h"
#include "..\epic\eos_ecom_types.h"
#include "..\epic\eos_auth.h"
#include "EgsAPI.h"

using namespace EgsInternal;

//EOS functions
static EOS_HPlatform(*ogEOS_Platform_Create)(const EOS_Platform_Options*) = nullptr;
static EOS_HEcom(*ogEOS_Platform_GetEcomInterface)(EOS_HPlatform) = nullptr;
static void(*ogEOS_Ecom_QueryEntitlements)(EOS_HEcom, const EOS_Ecom_QueryEntitlementsOptions*, void*, const EOS_Ecom_OnQueryEntitlementsCallback) = nullptr;
static unsigned int(*ogEOS_Ecom_GetEntitlementsCount)(EOS_HEcom, const EOS_Ecom_GetEntitlementsCountOptions*) = nullptr;
static EOS_EResult(*ogEOS_Ecom_CopyEntitlementByIndex)(EOS_HEcom, const EOS_Ecom_CopyEntitlementByIndexOptions*, EOS_Ecom_Entitlement**) = nullptr;
static void(*ogEOS_Ecom_Entitlement_Release)(EOS_Ecom_Entitlement*) = nullptr;

static void(*ogEOS_Ecom_QueryOffers)(EOS_HEcom, const EOS_Ecom_QueryOffersOptions*, void*, const EOS_Ecom_OnQueryOffersCallback) = nullptr;
static unsigned int(*ogEOS_Ecom_GetOfferCount)(EOS_HEcom, const EOS_Ecom_GetOfferCountOptions*) = nullptr;
static EOS_EResult(*ogEOS_Ecom_CopyOfferByIndex)(EOS_HEcom, const EOS_Ecom_CopyOfferByIndexOptions*, EOS_Ecom_CatalogOffer**) = nullptr;
static void(*ogEOS_Ecom_CatalogOffer_Release)(EOS_Ecom_CatalogOffer*) = nullptr;

static unsigned int(*ogEOS_Ecom_GetOfferItemCount)(EOS_HEcom, const EOS_Ecom_GetOfferItemCountOptions*) = nullptr;
static EOS_EResult(*ogEOS_Ecom_CopyOfferItemByIndex)(EOS_HEcom, const EOS_Ecom_CopyOfferItemByIndexOptions*, EOS_Ecom_CatalogItem**) = nullptr;
static void(*ogEOS_Ecom_CatalogItem_Release)(EOS_Ecom_CatalogItem*) = nullptr;

static void(*ogEOS_Ecom_QueryOwnership)(EOS_HEcom, const EOS_Ecom_QueryOwnershipOptions*, void*, const EOS_Ecom_OnQueryOwnershipCallback) = nullptr;

static void(*ogEOS_Auth_Login)(EOS_HAuth, const EOS_Auth_LoginOptions*, void*, const EOS_Auth_OnLoginCallback) = nullptr;
static EOS_EResult(*ogEOS_EpicAccountId_ToString)(EOS_EpicAccountId, char*, int*);

//Hooks and trampolines
static EOS_HPlatform(*ogEOS_Platform_Create_Trampoline)(const EOS_Platform_Options*) = nullptr;
static void(*ogEOS_Auth_Login_Trampoline)(EOS_HAuth, const EOS_Auth_LoginOptions*, void*, const EOS_Auth_OnLoginCallback) = nullptr;
static void(*ogEOS_Auth_OnLogicCallback)(const EOS_Auth_LoginCallbackInfo* Data) = nullptr;
static EOS_HPlatform EOS_Platform_Create_Hook(const EOS_Platform_Options* Options)
{
	if (ogEOS_Platform_Create_Trampoline)
	{
		pHPlatform = ogEOS_Platform_Create_Trampoline(Options);
		if (!pHPlatform)
		{
			eLog::Message(__FUNCTION__, "Failed to capture EOS Platform! Epic functions will not be available!");
			return nullptr;
		}
		eLog::Message("EgsInternal::Info()", "EOS Platform: %p", pHPlatform);

		pHEcom = ogEOS_Platform_GetEcomInterface(pHPlatform);
		eLog::Message("EgsInternal::Info()", "EOS Ecom: %p", pHEcom);
	}

	MH_DisableHook((void*)ogEOS_Platform_Create);
	return pHPlatform;
}
static void EOS_Auth_Login_Hook(EOS_HAuth Handle, const EOS_Auth_LoginOptions* Options, void* ClientData, const EOS_Auth_OnLoginCallback CompletionDelegate)
{
	if (ogEOS_Auth_Login_Trampoline)
	{
		ogEOS_Auth_OnLogicCallback = CompletionDelegate;

		auto OnLoginCallback = [](const EOS_Auth_LoginCallbackInfo* Data)
			{
				if (Data->ResultCode == EOS_EResult::EOS_Success)
				{
					pEpicAccountId = Data->LocalUserId;

					if (pEpicAccountId)
					{
						int size = sizeof(szUserId);

						ogEOS_EpicAccountId_ToString(pEpicAccountId, szUserId, &size);
						if (szUserId)
							eLog::Message("EgsInternal::Info()", "EOS User Id: %s", szUserId);
					}

					//Call into public API
					EgsAPI::Initialize();
				}

				ogEOS_Auth_OnLogicCallback(Data);
			};

		ogEOS_Auth_Login_Trampoline(Handle, Options, ClientData, OnLoginCallback);
	}

	MH_DisableHook((void*)ogEOS_Auth_Login);
}
int64 EGS_Setup_Hook(int64 a1, int64 a2, int64 a3)
{
	eLog::Message("EgsInternal::Info()", "Init");

	hEOS = GetModuleHandleA("EOSSDK-Win64-Shipping.dll");

	if (!hEOS)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to find EOSSDK module! Epic functions will not be available!");
		goto leave;
	}

	ogEOS_Platform_Create = (EOS_HPlatform(*)(const EOS_Platform_Options*))GetProcAddress(hEOS, "EOS_Platform_Create");
	if (!ogEOS_Platform_Create)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Platform_Create! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Platform_GetEcomInterface = (EOS_HEcom(*)(EOS_HPlatform))GetProcAddress(hEOS, "EOS_Platform_GetEcomInterface");
	if (!ogEOS_Platform_GetEcomInterface)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Platform_GetEcomInterface! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_QueryEntitlements = (void(*)(EOS_HEcom, const EOS_Ecom_QueryEntitlementsOptions*, void*, const EOS_Ecom_OnQueryEntitlementsCallback))GetProcAddress(hEOS, "EOS_Ecom_QueryEntitlements");
	if (!ogEOS_Ecom_QueryEntitlements)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_QueryEntitlements! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_GetEntitlementsCount = (unsigned int(*)(EOS_HEcom, const EOS_Ecom_GetEntitlementsCountOptions*))GetProcAddress(hEOS, "EOS_Ecom_GetEntitlementsCount");
	if (!ogEOS_Ecom_GetEntitlementsCount)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_GetEntitlementsCount! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_CopyEntitlementByIndex = (EOS_EResult(*)(EOS_HEcom, const EOS_Ecom_CopyEntitlementByIndexOptions*, EOS_Ecom_Entitlement**))GetProcAddress(hEOS, "EOS_Ecom_CopyEntitlementByIndex");
	if (!ogEOS_Ecom_CopyEntitlementByIndex)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_CopyEntitlementByIndex! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_Entitlement_Release = (void(*)(EOS_Ecom_Entitlement*))GetProcAddress(hEOS, "EOS_Ecom_Entitlement_Release");
	if (!ogEOS_Ecom_Entitlement_Release)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_Entitlement_Release! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_QueryOffers = (void(*)(EOS_HEcom, const EOS_Ecom_QueryOffersOptions*, void*, const EOS_Ecom_OnQueryOffersCallback))GetProcAddress(hEOS, "EOS_Ecom_QueryOffers");
	if (!ogEOS_Ecom_QueryOffers)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_QueryOffers! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_GetOfferCount = (unsigned int(*)(EOS_HEcom, const EOS_Ecom_GetOfferCountOptions*))GetProcAddress(hEOS, "EOS_Ecom_GetOfferCount");
	if (!ogEOS_Ecom_GetOfferCount)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_GetOfferCount! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_CopyOfferByIndex = (EOS_EResult(*)(EOS_HEcom, const EOS_Ecom_CopyOfferByIndexOptions*, EOS_Ecom_CatalogOffer**))GetProcAddress(hEOS, "EOS_Ecom_CopyOfferByIndex");
	if (!ogEOS_Ecom_CopyOfferByIndex)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_CopyOfferByIndex! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_CatalogOffer_Release = (void(*)(EOS_Ecom_CatalogOffer*))GetProcAddress(hEOS, "EOS_Ecom_CatalogOffer_Release");
	if (!ogEOS_Ecom_CatalogOffer_Release)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_CatalogOffer_Release! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_GetOfferItemCount = (unsigned int(*)(EOS_HEcom, const EOS_Ecom_GetOfferItemCountOptions*))GetProcAddress(hEOS, "EOS_Ecom_GetOfferItemCount");
	if (!ogEOS_Ecom_GetOfferItemCount)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_GetOfferItemCount! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_CopyOfferItemByIndex = (EOS_EResult(*)(EOS_HEcom, const EOS_Ecom_CopyOfferItemByIndexOptions*, EOS_Ecom_CatalogItem**))GetProcAddress(hEOS, "EOS_Ecom_CopyOfferItemByIndex");
	if (!ogEOS_Ecom_CopyOfferItemByIndex)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_CopyOfferItemByIndex! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_CatalogItem_Release = (void(*)(EOS_Ecom_CatalogItem*))GetProcAddress(hEOS, "EOS_Ecom_CatalogItem_Release");
	if (!ogEOS_Ecom_CatalogItem_Release)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_CatalogItem_Release! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Ecom_QueryOwnership = (void(*)(EOS_HEcom, const EOS_Ecom_QueryOwnershipOptions*, void*, const EOS_Ecom_OnQueryOwnershipCallback))GetProcAddress(hEOS, "EOS_Ecom_QueryOwnership");
	if (!ogEOS_Ecom_QueryOwnership)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Ecom_QueryOwnership! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_Auth_Login = (void(*)(EOS_HAuth, const EOS_Auth_LoginOptions*, void*, const EOS_Auth_OnLoginCallback))GetProcAddress(hEOS, "EOS_Auth_Login");
	if (!ogEOS_Auth_Login)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_Auth_Login! Epic functions will not be available!");
		goto leave;
	}
	ogEOS_EpicAccountId_ToString = (EOS_EResult(*)(EOS_EpicAccountId, char*, int*))GetProcAddress(hEOS, "EOS_EpicAccountId_ToString");
	if (!ogEOS_EpicAccountId_ToString)
	{
		eLog::Message(__FUNCTION__, "ERROR: Failed to retrieve EOS_EpicAccountId_ToString! Epic functions will not be available!");
		goto leave;
	}

	MH_CreateHook((void*)ogEOS_Platform_Create, &EOS_Platform_Create_Hook, (void**)&ogEOS_Platform_Create_Trampoline);
	MH_EnableHook((void*)ogEOS_Platform_Create);

	MH_CreateHook((void*)ogEOS_Auth_Login, &EOS_Auth_Login_Hook, (void**)&ogEOS_Auth_Login_Trampoline);
	MH_EnableHook((void*)ogEOS_Auth_Login);

	EgsInternal::bEgsInternalOk = true;
	eLog::Message("EgsInternal::Info()", "INFO: Init OK");

leave:
	if (ogEGS_Setup)
		return ogEGS_Setup(a1, a2, a3);

	return 0;
}

//API
void EgsInternal::QueryEntitlements(const EOS_Ecom_QueryEntitlementsOptions* Options, const EOS_Ecom_OnQueryEntitlementsCallback Callback)
{
	ogEOS_Ecom_QueryEntitlements(pHEcom, Options, nullptr, Callback);
}

void EgsInternal::QueryOffers(const EOS_Ecom_QueryOffersOptions* Options, const EOS_Ecom_OnQueryOffersCallback Callback)
{
	ogEOS_Ecom_QueryOffers(pHEcom, Options, nullptr, Callback);
}

void EgsInternal::QueryOwnership(const EOS_Ecom_QueryOwnershipOptions* Options, const EOS_Ecom_OnQueryOwnershipCallback Callback)
{
	ogEOS_Ecom_QueryOwnership(pHEcom, Options, nullptr, Callback);
}