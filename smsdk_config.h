#pragma once

#define SMEXT_CONF_NAME			"[L4D/2] Game Modes Restrictions"
#define SMEXT_CONF_DESCRIPTION	"Prevents players that are connecting through the lobby/matchmaking from being able to play game modes that the server isn't supporting"
#define SMEXT_CONF_VERSION		"1.0.0"
#define SMEXT_CONF_AUTHOR		"Justin \"Sir Jay\" Chellah"
#define SMEXT_CONF_URL			"https://justin-chellah.com"
#define SMEXT_CONF_LOGTAG		"L4D2-GMR"
#define SMEXT_CONF_LICENSE		"MIT"
#define SMEXT_CONF_DATESTRING	__DATE__

#define SMEXT_LINK(name) 		SDKExtension *g_pExtensionIface = name;

#define SMEXT_CONF_METAMOD