/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#define GAME_NAME "DDNet"
#define DDNET_VERSION_NUMBER 19040

#define GAME_RELEASE_VERSION "11.8"
#define CLIENT_VERSIONNR 11080

#define CHILLERBOT_VERSION "0004-dev"

#define DDBOT_VERSION "1.0.0-dev"
extern const char *GIT_SHORTREV_HASH;

#define CLIENT_VERSION7 0x0705
#define GAME_VERSION "0.6.4, " GAME_RELEASE_VERSION
#define GAME_NETVERSION "0.6 626fce9a778df4d4"
#define GAME_NETVERSION7 "0.7 802f1be60a05665f"

#ifndef GAME_RELEASE_VERSION_INTERNAL
#define GAME_RELEASE_VERSION_INTERNAL 11.8
#endif
#define GAME_RELEASE_VERSION STRINGIFY(GAME_RELEASE_VERSION_INTERNAL)

#endif
