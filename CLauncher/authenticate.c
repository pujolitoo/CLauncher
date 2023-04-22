#include "authenticate.h"
#include "weblogin.h"
#include "allocator.h"
#include <string.h>
#include <stdlib.h>
#include "downloader.h"
#include <cJSON.h>

#define RESCODE_SIZE 48

typedef struct
{
	HeapString token;
	HeapString hash;
}AuthResponse;

DownloadCallback cl = { 0 };

const char* xbox_auth_url = "https://user.auth.xboxlive.com/user/authenticate";
const char* xbox_auth_body =
"{"
	"\"Properties\": {"
		"\"AuthMethod\": \"RPS\","
		"\"SiteName\": \"user.auth.xboxlive.com\","
		"\"RpsTicket\": \"d=%s\""
	"},"
	"\"RelyingParty\": \"http://auth.xboxlive.com\","
	"\"TokenType\": \"JWT\""
"}";

const char* xsts_url = "https://xsts.auth.xboxlive.com/xsts/authorize";
const char* xsts_body =
"{"
	"\"Properties\": {"
		"\"SandboxId\": \"RETAIL\","
		"\"UserTokens\": ["
			"\"%s\""
		"]"
	"},"
	"\"RelyingParty\": \"rp://api.minecraftservices.com/\","
	"\"TokenType\": \"JWT\""
"}";

const char* mc_auth_url = "https://api.minecraftservices.com/authentication/login_with_xbox";
const char* mc_auth_body =
"{"
"\"identityToken\": \"XBL3.0 x=%s;%s\""
"}";

const char* mc_ownership_url = "https://api.minecraftservices.com/entitlements/mcstore";

const char* mc_profile_url = "https://api.minecraftservices.com/minecraft/profile";

MinecraftProfile g_Profile = { 0 };
bool authenticated = false;

struct curl_slist* GetJSONHeaders()
{
	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "Accept: application/json");
	return headers;
}

cJSON* GetAccessToken(const char* code, int refresh)
{
	struct curl_slist* hList = NULL;
	hList = curl_slist_append(hList, "Content-Type: application/x-www-form-urlencoded");

	char tokenURL[1050] = { 0 };

	if (refresh)
	{
		sprintf_s(tokenURL, 1050, "client_id=71584533-73c0-40b4-b4b2-9ce9f90ac993&scope=XboxLive.signin offline_access&refresh_token=%s&redirect_uri=https://login.live.com/oauth20_desktop.srf&grant_type=refresh_token", code);
	}
	else
	{
		sprintf_s(tokenURL, 1050, "client_id=71584533-73c0-40b4-b4b2-9ce9f90ac993&scope=XboxLive.signin offline_access&code=%s&redirect_uri=https://login.live.com/oauth20_desktop.srf&grant_type=authorization_code", code);
	}

	RDATA response = PostRequest(
		"https://login.microsoftonline.com/consumers/oauth2/v2.0/token?",
		hList,
		tokenURL
	);

	cJSON* hResponse = cJSON_Parse(response.data);
	ClearData(response);

	curl_slist_free_all(hList);

	return hResponse;
}

AuthResponse RequestAuthentication(const char* url, const char* body, const char* token)
{
	AuthResponse out = { 0 };

	//Construct body
	size_t bufferSize = strlen(token) + strlen(body) + 1;
	char* buffer = malloc(bufferSize);
	sprintf_s(buffer, bufferSize, body, token);

	//POST and GET
	struct curl_slist* headers = GetJSONHeaders();

	RDATA resp = PostRequest(url, headers, buffer);

	free(buffer);
	curl_slist_free_all(headers);

	cJSON* json = cJSON_Parse(resp.data);

	out.token = createString(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "Token")));
	out.hash = createString(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(json, "DisplayClaims"), "xui"), 0), "uhs")));

	cJSON_Delete(json);
	ClearData(resp);

	return out;
}

AuthResponse GetXBLToken(const char* accessToken)
{
	return RequestAuthentication(xbox_auth_url, xbox_auth_body, accessToken);
}

AuthResponse GetXSTSToken(const char* XBL)
{
	return RequestAuthentication(xsts_url, xsts_body, XBL);
}

cJSON* GetMinecraftResponse(AuthResponse xsts)
{
	struct curl_slist* headers = GetJSONHeaders();

	size_t bufferSize = strlen(xsts.token.string) + strlen(mc_auth_body) + strlen(xsts.hash.string) + 1;
	char* buffer = malloc(bufferSize);
	sprintf_s(buffer, bufferSize, mc_auth_body, xsts.hash.string, xsts.token.string);

	RDATA resp = PostRequest(mc_auth_url, headers, buffer);
	free(buffer);

	return cJSON_Parse(resp.data);
}

int OwnsMinecraft(const char* accessToken)
{
	int hasMinecraft = 1;

	struct curl_slist* headers = NULL;
	HeapString authHeader = join("Authorization: Bearer ", accessToken, NULL);

	headers = curl_slist_append(headers, authHeader.string);

	RDATA data = GetDataHeaders(mc_ownership_url, cl, headers);
	cJSON* parsed = cJSON_Parse(data.data);

	if (!cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(parsed, "items")))
	{
		hasMinecraft = 0;
	}

	cJSON_Delete(parsed);
	curl_slist_free_all(headers);
	cleanString(&authHeader, false);

	return hasMinecraft;
}

cJSON* GetMCProfile(const char* const access_token)
{
	struct curl_slist* headers = NULL;
	HeapString authHeader = join("Authorization: Bearer ", access_token, NULL);
	headers = curl_slist_append(headers, authHeader.string);

	RDATA profileRes = GetDataHeaders(mc_profile_url, cl, headers);

	cJSON* json = cJSON_Parse(profileRes.data);
	CLEANSTRING(authHeader);
	ClearData(profileRes);
	curl_slist_free_all(headers);
	return json;
}

void ClearAuthResponse(AuthResponse t)
{
	CLEANSTRING(t.hash);
	CLEANSTRING(t.token);
}

int Authenticate(const char* const refresh_token)
{
	ClearProfile();
	char resToken[255] = {0};

	HANDLE thread;
	DWORD threadID = 0;
	cJSON* accessToken = NULL;

	if (!refresh_token)
	{
		//WebLogin
		thread = CreateThread(NULL, 0, (void*)RunWeb, (void*)resToken, 0, &threadID);
		if (!thread)
		{
			return 1;
		}
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		accessToken = GetAccessToken(resToken, 0);
	}
	else
	{
		accessToken = GetAccessToken(refresh_token, 1);
		if (cJSON_GetObjectItem(accessToken, "error"))
		{
			return (-1);
		}
	}

	g_Profile.refresh_token = createString(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(accessToken, "refresh_token")));

	AuthResponse xblToken = GetXBLToken(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(accessToken, "access_token")));

	AuthResponse xstsToken = GetXSTSToken(xblToken.token.string);

	cJSON* minecraftInfo = GetMinecraftResponse(xstsToken);

	ClearAuthResponse(xblToken);
	ClearAuthResponse(xstsToken);

	const char* realAccessToken = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(minecraftInfo, "access_token"));

	if (OwnsMinecraft(realAccessToken))
	{
		g_Profile.access_token = createString(realAccessToken);

		cJSON* profile = GetMCProfile(realAccessToken);

		g_Profile.id = createString(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(profile, "id")));
		g_Profile.name = createString(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(profile, "name")));
		g_Profile.type = MICROSOFT_ACCOUNT;

		cJSON_Delete(profile);
	}
	else
	{
		MessageBoxW(NULL, L"This microsoft account doesn't own minecraft.", L"Error", MB_ICONERROR);
		return 1;
	}

	authenticated = true;

	cJSON_Delete(minecraftInfo);
	cJSON_Delete(accessToken);

	return 0;
}

int OfflineAuthentication(const char* username)
{
	if (authenticated)
	{
		ClearProfile();
	}
	g_Profile.type = OFFLINE_ACCOUNT;
	g_Profile.name = createString(username);
}

const MinecraftProfile* const GetProfile()
{
	if (!authenticated) return NULL;
	return &g_Profile;
}

int IsLoggedIn()
{
	return authenticated;
}

void ClearProfile()
{
	if (g_Profile.id.string)
	{
		CLEANSTRING(g_Profile.id);
	}
	if (g_Profile.name.string)
	{
		CLEANSTRING(g_Profile.name);
	}
	if (g_Profile.access_token.string)
	{
		CLEANSTRING(g_Profile.access_token);
	}
	if (g_Profile.refresh_token.string)
	{
		CLEANSTRING(g_Profile.refresh_token);
	}
	memset(&g_Profile, 0, sizeof(MinecraftProfile));
}