#pragma once
#include "..\utils.h"
#include "..\epic\eos_types.h"
#include "..\epic\eos_common.h"
#include "..\epic\eos_ecom_types.h"

namespace EgsInternal
{
	inline HMODULE hEOS = nullptr;
	inline EOS_HPlatform pHPlatform = nullptr;
	inline EOS_HEcom pHEcom = nullptr;
	inline EOS_EpicAccountId pEpicAccountId = nullptr;
	inline char szUserId[EOS_EPICACCOUNTID_MAX_LENGTH + 1] = {};
	inline bool bEgsInternalOk = false;

	void QueryEntitlements(const EOS_Ecom_QueryEntitlementsOptions* Options, const EOS_Ecom_OnQueryEntitlementsCallback Callback);
	void QueryOffers(const EOS_Ecom_QueryOffersOptions* Options, const EOS_Ecom_OnQueryOffersCallback Callback);
	void QueryOwnership(const EOS_Ecom_QueryOwnershipOptions* Options, const EOS_Ecom_OnQueryOwnershipCallback Callback);
}

int64 EGS_Setup_Hook(int64 a1, int64 a2, int64 a3);
inline int64(*ogEGS_Setup)(int64, int64, int64) = nullptr;