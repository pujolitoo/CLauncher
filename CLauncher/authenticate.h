#pragma once
#ifndef _H_AUTHENTICATE_
#define _H_AUTHENTICATE_

#include "strutils.h"
#include "Core.h"

typedef void(*stateChanged)(const char* message);

typedef enum
{
	MICROSOFT_ACCOUNT,
	OFFLINE_ACCOUNT
}AccountType;

typedef struct
{
	HeapString id;
	HeapString name;
	HeapString access_token;
	HeapString refresh_token;
	AccountType type;
}MinecraftProfile;


EXTERNC int Authenticate(const char* const refresh_token);

int OfflineAuthentication(const char* username);

EXTERNC const MinecraftProfile* const GetProfile();

int IsLoggedIn();

EXTERNC void ClearProfile();

#endif //_H_AUTHENTICATE_