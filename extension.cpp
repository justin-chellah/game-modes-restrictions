#include "extension.h"
#include "../imatchext/IMatchExtInterface.h"
#include "fmtstr.h"
#include <vstdlib/random.h>
#include <vstdlib/IKeyValuesSystem.h>

CGameModesRestrictionsExt g_GameModesRestrictionsExt;

SMEXT_LINK(&g_GameModesRestrictionsExt);

IMatchExtInterface *imatchext = nullptr;
ICvar *g_pCVar = nullptr;

SH_DECL_HOOK1_void(IServerGameDLL, ApplyGameSettings, SH_NOATTRIB, 0, KeyValues *);

bool CGameModesRestrictionsExt::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "imatchext.ext", true, true);

	return true;
}

void CGameModesRestrictionsExt::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(IMATCHEXT, imatchext);
}

bool CGameModesRestrictionsExt::SDK_OnMetamodLoad(ISmmAPI * ismm, char * error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

	SH_ADD_HOOK(IServerGameDLL, ApplyGameSettings, gamedll, SH_MEMBER(this, &CGameModesRestrictionsExt::ApplyGameSettings), false);

	return true;
}

bool CGameModesRestrictionsExt::SDK_OnMetamodUnload(char *error, size_t maxlength)
{
	SH_REMOVE_HOOK(IServerGameDLL, ApplyGameSettings, gamedll, SH_MEMBER(this, &CGameModesRestrictionsExt::ApplyGameSettings), false);

	return true;
}

bool CGameModesRestrictionsExt::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(IMATCHEXT, imatchext);

	return true;
}

void CGameModesRestrictionsExt::ApplyGameSettings(KeyValues *pRequest)
{
	SET_META_RESULT(MRES_IGNORED);

	if (pRequest)
	{
		const char *pszRequestGameMode = pRequest->GetString("Game/mode");

		if (!pszRequestGameMode[0])
		{
			return;
		}

		static ConVarRef sv_gametypes("sv_gametypes");
		static ConVarRef mp_gamemode("mp_gamemode");

		const char *pszGameMode = mp_gamemode.GetString();
 
		if (!V_stricmp(pszRequestGameMode, pszGameMode))
		{
			smutils->LogMessage(myself, "Won't override \"mp_gamemode\". Reason: Current game mode is the same as the requested one");

			return;
		}

		bool bOverrideMode = false;

		std::vector<std::string>& vecGameTypes = ke::Split(sv_gametypes.GetString(), ",");

		for (const std::string& i : vecGameTypes)
		{
			const char *pszGameType = i.c_str();

			// Mode is OK
			if (!V_stricmp(pszRequestGameMode, pszGameType))
			{
				return;
			}

			bOverrideMode = true;
		}

		if (bOverrideMode)
		{
			// Allow the same campaign for player convenience
			KeyValues *pSettings = new KeyValues(nullptr);
			KeyValues::AutoDelete autodelete(pSettings);
			pSettings->SetString("Game/mode", pszGameMode);
			pSettings->SetString("Game/campaign", pRequest->GetString("Game/campaign"));
			pSettings->SetInt("Game/chapter", 1);

			if (imatchext->GetIMatchExtL4D()->GetMapInfo(pSettings))
			{
				smutils->LogMessage(
					myself, 
					"Overridden requested not-allowed game mode \"%s\" with \"mp_gamemode\" value \"%s\" but allowed the same mission", 
					pszRequestGameMode, 
					pszGameMode);

				pRequest->SetString("Game/mode", pszGameMode);
				pRequest->SetInt("Game/chapter", 1);
			}
			else
			{
				KeyValues *pRandomMission = GetRandomNonWorkshopMission(pszGameMode);
				
				if (!pRandomMission)
				{
					return;
				}

				smutils->LogMessage(
					myself, 
					"Overridden requested not-allowed game mode \"%s\" with \"mp_gamemode\" value \"%s\" and adjusted mission settings accordingly", 
					pszRequestGameMode, 
					pszGameMode);

				pRequest->SetString("Game/mode", pszGameMode);
				pRequest->SetString("Game/campaign", pRandomMission->GetName());
				pRequest->SetInt("Game/chapter", 1);
			}
		}
	}
}

KeyValues *CGameModesRestrictionsExt::GetRandomNonWorkshopMission(const char *pszRequestGameMode) const
{
	KeyValues *pAllMissions = imatchext->GetIMatchExtL4D()->GetAllMissions();

	if (!pAllMissions)
	{
		return nullptr;
	}

	CUtlVector<KeyValues *> vecMissions;

	for (KeyValues *pMission = pAllMissions->GetFirstTrueSubKey(); pMission; pMission = pMission->GetNextTrueSubKey())
	{
		// We don't know if the joining players own this custom campaign and we don't want them to get dropped
		if (pMission->GetInt("addon"))
		{
			continue;
		}

		CFmtStr fmtChapters("modes/%s/chapters", pszRequestGameMode);
		if (!pMission->GetInt(fmtChapters))
		{
			continue;
		}

		vecMissions.AddToTail(pMission);
	}

	if (vecMissions.Count())
	{
		int iMission = RandomInt(0, vecMissions.Count() - 1);

		return vecMissions.Element(iMission);
	}

	return nullptr;
}