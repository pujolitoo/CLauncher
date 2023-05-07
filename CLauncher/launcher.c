#include "launcher.h"

#include <cJSON.h>
#include "downloader.h"
#include "main.h"
#include "arrutil.h"
#include "uuid4.h"
#include "ziphelper.h"
#include "process.h"
#include "osutil.h"

#define GenericError(x) MessageBoxA(NULL, x, "Error", MB_ICONERROR)

#define MANIFEST_URL "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"
#define RESOURCE_BASE_URL "https://resources.download.minecraft.net/"
#define JRE_RUNTIME_URL "https://api.adoptopenjdk.net/v3/assets/feature_releases/"
#define JRE_RUNTIME_URL_TERMINATION "/ga"
#define JRE_DISTRIBUTOR_TERMINATION "_adopt"

HANDLE hThread = NULL;
HANDLE hForgeThread = NULL;

HeapString mPath;
HeapString assetsFolder;
HeapString versionsFolder;
HeapString librariesFolder;
HeapString binariesFolder;
HeapString configFolder;
HeapString versionID;
HeapString versionIDBase;
HeapString loggingInfo;
LauncherCallback clback = {0};
HPArray classPath = {0};

const char* baseurl = "https://maven.minecraftforge.net/net/minecraftforge/forge/%s/forge-%s-installer.jar";

static DownloadCallback cl = {0};

void CreateBlankFile(const char* const path)
{
	FILE* handle;
	fopen_s(&handle, path, "wb");
	fclose(handle);
}

static void dlError(const char* errstr)
{
	fprintf_s(stderr, "Download Error: %s.\n", errstr);
}

void SetCallback(const LauncherCallback callback)
{
	clback = callback;
}

static void SetProgress(double value, double max)
{
	if (clback.clProgress)
	{
		clback.clProgress(value, max);
	}
}

static void CallbackComplete()
{
	if (clback.clCompleted)
	{
		clback.clCompleted();
	}
}

static void Log(const char* const message)
{
	if (clback.clLog)
	{
		clback.clLog(message);
	}
}

static void SetState(const char* const message)
{
	if (clback.clState)
	{
		clback.clState(message);
	}
}

static bool FileExists(const char* path)
{
	FILE* file;
	if (fopen_s(&file, path, "rb"))
	{
		return false;
	}
	else
	{
		fclose(file);
		return true;
	}
	return false;
}

static bool checkRules(cJSON* rulesArray)
{
	if (!rulesArray) return true;
	bool result = true;
	for (int i = 0; i < cJSON_GetArraySize(rulesArray); i++)
	{
		cJSON* obj = cJSON_GetArrayItem(rulesArray, i);
		const char* action = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, "action"));
		const char* osname = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(rulesArray, "os"), "name"));
		const char* arch = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, "arch"));
		bool noInfo = !arch && !osname;
		bool allowed = !strcmp(action, "allow");
		if (noInfo)
			result = allowed;
		else if(!noInfo)
		{
			if (osname)
			{
				if (!strcmp(OSNAME, osname))
				{
					result = allowed;
				}
			}
		}
		continue;
	}
	return result;
}

int InstallLibraries(cJSON* versionMetadata)
{
	// Install libraries
	Log("Installing libraries...");
	cJSON* arrlib = cJSON_GetObjectItemCaseSensitive(versionMetadata, "libraries");
	for (int i = 0; i < cJSON_GetArraySize(arrlib); i++)
	{

		bool download = true;
		bool isnative = false;

		cJSON* lib = cJSON_GetArrayItem(arrlib, i);
		const char* libname = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(lib, "name"));
		HeapString nativesfilepath = { 0 };
		HeapString libfilepath = join(
			librariesFolder.string,
			"/",
			cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(lib, "downloads"), "artifact"), "path")),
			NULL
		);
		const char* nativesurl = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(lib, "downloads"), "classifiers"), "natives-"OSNAME), "url"));
		const char* nativespath = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(lib, "downloads"), "classifiers"), "natives-"OSNAME), "path"));
		if (nativespath)
		{
			nativesfilepath = join(
				librariesFolder.string,
				"/",
				nativespath,
				NULL
			);
			NORMALIZE(nativesfilepath);
			HeapString nativepath = pathFromFile(nativesfilepath);
			RECDIRS(nativepath.string);
			CLEANSTRING(nativepath);
		}

		NORMALIZE(libfilepath);
		HeapString libpath = pathFromFile(libfilepath);

		RECDIRS(libpath.string);

		const char* liburl = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(lib, "downloads"), "artifact"), "url"));

		cJSON* rules = cJSON_GetObjectItemCaseSensitive(lib, "rules");
		download = checkRules(rules);

		int res = 0;

		if (download)
		{
			if (!FileExists(libfilepath.string))
			{
				res = DirectDowload(liburl, libfilepath.string);
				// LOG LIBS
				HeapString log = join("Installed: ", libname, NULL);
				Log(log.string);
				SetProgress(i + 1, cJSON_GetArraySize(arrlib));
				CLEANSTRING(log);
			}
			addToHPArray(&classPath, libfilepath.string);
			if (nativesfilepath.string)
			{
				if (!FileExists(nativesfilepath.string))
				{
					DirectDowload(nativesurl, nativesfilepath.string);
				}
				addToHPArray(&classPath, nativesfilepath.string);
			}
		}

		if (res == -1)
		{
			printf("Error while saving library: %s\n", libname);
			return -1;
		}

		CLEANSTRING(nativesfilepath);
		cleanString(&libfilepath, false);
		cleanString(&libpath, false);
	}

	// Download client

	HeapString clientPath = join(versionsFolder.string, "/", versionID.string, "/", versionID.string, ".jar", NULL);
	NORMALIZE(clientPath);

	const char* clientURL = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(versionMetadata, "downloads"), "client"), "url"));
	RDATA clientData = GetData(clientURL, cl);
	SaveBinToFile(clientPath.string, clientData);
	addToHPArray(&classPath, clientPath.string);

	cleanString(&clientPath, false);
	ClearData(clientData);

	loggingInfo = join(configFolder.string, "/", cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(versionMetadata, "logging"), "client"), "file"), "id")), NULL);
	NORMALIZE(loggingInfo);

	if(!FileExists(loggingInfo.string)) DirectDowload(
		cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(versionMetadata, "logging"), "client"), "file"), "url")),
		loggingInfo.string
	);
	return 0;
}

int InstallAssets(cJSON* versionMetadata)
{
	Log("Installing assets...");
	HeapString indexPath = join(assetsFolder.string, "/indexes/", versionIDBase.string, ".json", NULL);
	HeapString indexfileDir = pathFromFile(indexPath);
	NORMALIZE(indexfileDir);
	RECDIRS(indexfileDir.string);
	const char* assetsURL = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(versionMetadata, "assetIndex"), "url"));
	RDATA indexData = GetData(assetsURL, cl);
	SaveBinToFile(indexPath.string, indexData);
	HeapString indexString = GetDataAsString(indexData);
	cJSON* indexMetadata = cJSON_Parse(indexString.string);
	ClearData(indexData);
	cleanString(&indexString, false);
	cleanString(&indexPath, false);
	cleanString(&indexfileDir, false);

	int offset = 1;
	int assets = cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(indexMetadata, "objects"));

	cJSON* objectiterator = cJSON_GetObjectItemCaseSensitive(indexMetadata, "objects")->child;
	while (objectiterator != NULL)
	{
		const char* hash = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(objectiterator, "hash"));
		HeapString initials = getSubString(hash, 0, 1);
		HeapString assetURL = join(RESOURCE_BASE_URL, initials.string, "/", hash, NULL);
		HeapString assetDir = join(assetsFolder.string, "/objects/", initials.string, "/", NULL);
		NORMALIZE(assetDir);
		RECDIRS(assetDir.string);
		HeapString assetPath = join(assetDir.string, hash, NULL);

		if (!FileExists(assetPath.string)) {
			RDATA assetData = GetDataS(assetURL.string, cl, objectiterator->child->next->valueint);
			SaveBinToFile(assetPath.string, assetData);
			ClearData(assetData);
		}

		// LOG ASSETS
		SetState(hash);
		SetProgress(offset, assets);

		cleanString(&assetURL, false);
		cleanString(&assetDir, false);
		cleanString(&assetPath, false);
		cleanString(&initials, false);

		objectiterator = objectiterator->next;
		offset++;
	}
	cJSON_Delete(indexMetadata);
	return 0;
}

int InstallVersion(void* param);

HeapString InstallJRE()
{
	//Download JRE
	/*
	char javaVersion[5];
	ZeroMemory(javaVersion, sizeof(javaVersion));
	_itoa_s(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(versionMetadata, "javaVersion"), "majorVersion")),
		javaVersion,
		sizeof(javaVersion),
		10
	);

	HeapString jreMetaURL = join(JRE_RUNTIME_URL, javaVersion, JRE_RUNTIME_URL_TERMINATION, NULL);
	RDATA jreMetaRaw = GetData(jreMetaURL.string, cl);
	cJSON* jreMetadata = cJSON_Parse(jreMetaRaw.data);
	ClearData(jreMetaRaw);

	HeapString javaBase;
	HeapString jrePackPath;
	HeapString javaPath;

	cJSON* binaries = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(jreMetadata, 0), "binaries");
	for (int i = 0; i < cJSON_GetArraySize(binaries); i++)
	{
		cJSON* binary = cJSON_GetArrayItem(binaries, i);
		const char* os = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(binary, "os"));
		const char* ref = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(binary, "scm_ref"));
		const char* buildType = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(binary, "image_type"));
		const char* packageURL = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(binary, "package"), "link"));
		const char* packageName = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(binary, "package"), "name"));
		uint64_t packageSize = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(binary, "package"), "size"));

		if (!strcmp(os, OSNAME) && !strcmp(buildType, "jre"))
		{
			HeapString temp = getSubString(ref, 0, (strlen(ref) - strlen(JRE_DISTRIBUTOR_TERMINATION)) - 1);
			javaBase = join(temp.string, "-", buildType, NULL);
			jrePackPath = join(runtimeFolder.string, "/", packageName, NULL);
			NORMALIZE(jrePackPath);
			RDATA jreData = GetDataS(packageURL, cl, packageSize);
			SaveBinToFile(jrePackPath.string, jreData);
			ClearData(jreData);
			cleanString(&temp, false);
			break;
		}
	}

	javaPath = createString("java");

	if (ExtractAll(jrePackPath.string, runtimeFolder.string) != 0)
	{
		puts("JRE couln't be downloaded, using java path version.");
	}
	else
	{
		cleanString(&javaPath, false);
		javaPath = join(runtimeFolder.string, "/", javaBase.string, "/bin/", "java.exe", NULL);
		NORMALIZE(javaPath);
	}

	remove(jrePackPath.string);

	cleanString(&jrePackPath, false);
	cJSON_Delete(jreMetadata);
	*/
}

int InitMinecraftInstance(const char* const path)
{
	mPath = createString(path);
	assetsFolder = join(path, "/assets", NULL);
	versionsFolder = join(path, "/versions", NULL);
	librariesFolder = join(path, "/libraries", NULL);
	binariesFolder = join(path, "/bin", NULL);
	configFolder = join(path, "/config", NULL);

	NORMALIZE(assetsFolder);
	NORMALIZE(versionsFolder);
	NORMALIZE(librariesFolder);
	NORMALIZE(binariesFolder);
	NORMALIZE(configFolder);

	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InstallVersion, NULL, CREATE_SUSPENDED, NULL);
}

void ClearMinecraftInstance()
{
	memset(&clback, 0, sizeof(LauncherCallback));
	cleanHPArray(&classPath);
	CLEANSTRING(assetsFolder);
	CLEANSTRING(versionsFolder);
	CLEANSTRING(librariesFolder);
	CLEANSTRING(binariesFolder);
	CLEANSTRING(configFolder);
	CLEANSTRING(versionID);
	CLEANSTRING(versionIDBase);
	CloseHandle(hThread);
}

int InstallVersion(void* param)
{
	clback.clLog("Installing...");
	RECDIRS(mPath.string);
	RECDIRS(assetsFolder.string);
	RECDIRS(versionsFolder.string);
	RECDIRS(librariesFolder.string);
	RECDIRS(binariesFolder.string);
	RECDIRS(configFolder.string);

	HeapString profile = join(mPath.string, "/launcher_profiles.json", NULL);
	CreateBlankFile(profile.string);
	CLEANSTRING(profile);

	// Base version
	char baseVersion[8] = { 0 };
	int copySize = versionID.size;
	if (countChars(versionID.string, '.') > 1)
	{
		copySize = findCharBackwards(versionID.string, '.');
	}

	memcpy_s(
		baseVersion,
		8,
		versionID.string,
		copySize
	);

	versionIDBase = createString(baseVersion);

	HeapString vMetadataFilePath;
	cJSON* versionMetadata = NULL;

	// Manifest
	RDATA vMan = GetData(MANIFEST_URL, cl);
	cJSON* manifest = cJSON_Parse(vMan.data);
	ClearData(vMan);

	cJSON* versionArray = cJSON_GetObjectItemCaseSensitive(manifest, "versions");

	// Version manifest
	for (int i = 0; i < cJSON_GetArraySize(versionArray); i++)
	{
		cJSON* arrItem = cJSON_GetArrayItem(versionArray, i);
		char* vID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(arrItem, "id"));
		if (!strcmp(vID, versionID.string))
		{
			char* versionMetaURL = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(arrItem, "url"));
			if (versionMetaURL)
			{
				HeapString path = join(versionsFolder.string, "/", vID, NULL);
				RECDIRS(path.string);

				vMetadataFilePath = join(path.string, "/", vID, ".json", NULL);

				RDATA vMeta = GetData(versionMetaURL, cl);
				versionMetadata = cJSON_Parse(vMeta.data);
				SaveBinToFile(vMetadataFilePath.string, vMeta);
				ClearData(vMeta);

				cleanString(&path, false);
			}
			break;
		}
	}

	if (!(&vMetadataFilePath) || !versionMetadata)
	{
		return -1;
	}
	cJSON_Delete(manifest);


	InstallLibraries(versionMetadata);
	InstallAssets(versionMetadata);

	Log("Installation Completed.");
	CallbackComplete();

	cJSON_Delete(versionMetadata);
}

int InstallVersionThreaded(const char* const version)
{
	versionID = createString(version);
	if (hThread)
	{
		ResumeThread(hThread);
	}
}

static inline int GetForgeURL(const char* const forgeVersion, char* buffer, size_t bufferSize)
{
	return sprintf_s(buffer, bufferSize, baseurl, forgeVersion, forgeVersion);
}

void logAdapter(const char* const message, size_t size_t)
{
	Log(message);
}

int InstallForge(const char* const fVersion)
{
	char* parsedUrl[MAX_PATH];

	Log("Installing Forge.");
	SetState("Installing Forge.");

	if(GetForgeURL(fVersion, parsedUrl, MAX_PATH) == -1)
	{
		return 0;
	}

	HeapString dlpath = join(GetEXEBasePath()->string, "/jars/forge-", fVersion, "-installer.jar", NULL);
	DirectDowload(parsedUrl, dlpath.string);

	HeapString forgeclipath = join(GetEXEBasePath()->string, "/jars/ForgeCLI-1.0.1-all.jar", NULL);

	HeapString command = join("javaw -jar ", forgeclipath.string, " --installer ", dlpath.string, " --target ", mPath.string, NULL);

	ExecuteProcess(NULL, command.string, logAdapter);

	CLEANSTRING(dlpath);
	CLEANSTRING(forgeclipath);
	CLEANSTRING(command);
	SetState("");
	return 1;
}

int InstallForgeThreaded(const char* const forge_version)
{
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InstallForge, forge_version, 0, NULL);
}

MCArgument* GetCmdLineArguments(cJSON* versionMetadata, int* nElem)
{
	MCArgument* list = NULL;
	size_t currListSize = 0;
	int count = 0;
	cJSON* jvmArgList = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(versionMetadata, "arguments"), "jvm");
	cJSON* gmArgList = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(versionMetadata, "arguments"), "game");

	for (int i = 0; i < cJSON_GetArraySize(jvmArgList); i++)
	{
		cJSON* item = cJSON_GetArrayItem(jvmArgList, i);
		if (checkRules(cJSON_GetObjectItemCaseSensitive(item, "rules")))
		{
			HeapString values = createString("");
			void* temp = list;
			currListSize += sizeof(MCArgument);
			list = (MCArgument*)realloc(list, currListSize);

			if (temp)
				free(temp);

			if (cJSON_GetObjectItemCaseSensitive(item, "value"))
			{
				for (int j = 0; j < cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(item, "value")); j++)
				{
					concatFromC(&values, cJSON_GetStringValue(cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(item, "value"), j)));
					concatFromC(&values, " ");
				}
			}
			else
			{
				concatFromC(&values, cJSON_GetStringValue(cJSON_GetArrayItem(jvmArgList, i)));
			}

			list[count] = (MCArgument)
			{
				createString(NULL),
				values
			};

			count++;
		}
	}

	int argC = 0;

	while (argC < cJSON_GetArraySize(gmArgList))
	{
		cJSON* item = cJSON_GetArrayItem(gmArgList, argC);
		const char* arg = cJSON_GetStringValue(item);
		if (!arg)
			break;


		count++;
		void* temp = list;
		currListSize += sizeof(MCArgument);
		list = (MCArgument*)realloc(list, currListSize);
		if (temp)
			free(temp);
		list[count] = (MCArgument)
		{
			createString(arg),
			createString(NULL)
		};
		argC += 2;
	}

	*nElem = count;
}

int ChangeArgumentValue(MCArgument* arguments, int nElem, const char* const argName, const char* const value)
{
	if (!arguments || nElem <= 0)
		return -1;
	for (int i = 0; i < nElem; i++)
	{
		if(!strcmp(arguments[i].argument.string, argName))
		{
			CLEANSTRING(arguments[i].value);
			arguments[i].value = createString(value);
		}
	}
}

void FreeArgList(MCArgument* list, int nElem)
{
	if (!list || nElem <= 0)
		return;
	for (int i = 0; i < nElem; i++)
	{
		CLEANSTRING(list[i].argument);
		CLEANSTRING(list[i].value);
	}
	free(list);
}

HeapString GetCommand(const struct launcher_info_t info)
{

	int nArgs = 0;
	HeapString versionManifest = join(versionsFolder.string, "/", info.versionID, "/", info.versionID, ".json", NULL);
	NORMALIZE(versionManifest);

	FILE* vFile = NULL;
	cJSON* parsedManifest = NULL;
	if (fopen_s(&vFile, versionManifest.string, "rb"))
	{
		if (vFile)
		{
			fseek(vFile, 0, SEEK_END);
			long fileSize = ftell(vFile);
			fseek(vFile, 0, SEEK_SET);
			char* temp = (char*)malloc(fileSize);
			fread_s(temp, fileSize, 1, fileSize, vFile);
			parsedManifest = cJSON_Parse(temp);
			free(temp);
		}
	}



	UUID4_T uuid;
	UUID4_STATE_T state;
	char playerUUID[UUID4_STR_BUFFER_SIZE];

	//Get the classpath
	HeapString cp = GetStringSep(classPath, ';');

	// Generate player uuid
	uuid4_seed(&state);
	uuid4_gen(&state, &uuid);
	if (!uuid4_to_s(uuid, playerUUID, sizeof(playerUUID)))
	{
		return;
	}

	//Generate command

	char memstr[255];
	_ultoa_s(info.memalloc, memstr, sizeof(memstr), 10);

	HPArray cmdargs = { 0 };

	addToHPArray(&cmdargs, join(info.javapath, " ", NULL).string);
	addToHPArray(&cmdargs, join("-Xmx", memstr, "M", " ", NULL).string);
	addToHPArray(&cmdargs, join("-XX:+UnlockExperimentalVMOptions -XX:+UseG1GC -XX:G1NewSizePercent=20 -XX:G1ReservePercent=20 -XX:MaxGCPauseMillis=50 -XX:G1HeapRegionSize=32M", " ", NULL).string);
	addToHPArray(&cmdargs, join("-Dlog4j.configurationFile=", loggingInfo.string, " ", NULL).string);
	addToHPArray(&cmdargs, join("-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump", " ", NULL).string);
	addToHPArray(&cmdargs, join("-Djava.library.path=", "", " ", NULL).string);
	addToHPArray(&cmdargs, join("-Dminecraft.launcher.brand=java-minecraft-launcher -Dminecraft.launcher.version=unknown", " ", NULL).string);
	addToHPArray(&cmdargs, join("-cp ", cp.string, " ", NULL).string); // Classpath
	addToHPArray(&cmdargs, join("net.minecraft.client.main.Main", " ", NULL).string); // Main class
	addToHPArray(&cmdargs, join("--username ", info.profile->name.string, " ", NULL).string);
	addToHPArray(&cmdargs, join("--version ", versionID.string, " ", NULL).string);
	addToHPArray(&cmdargs, join("--gameDir ", mPath.string, " ", NULL).string);
	addToHPArray(&cmdargs, join("--assetsDir ", assetsFolder.string, " ", NULL).string);
	addToHPArray(&cmdargs, join("--assetIndex ", versionIDBase.string, " ", NULL).string);
	if (info.profile->type == MICROSOFT_ACCOUNT)
	{
		addToHPArray(&cmdargs, join("--uuid ", info.profile->id.string, " ", NULL).string);
		addToHPArray(&cmdargs, join("--accessToken ", info.profile->access_token.string, " ", NULL).string);
		addToHPArray(&cmdargs, join("--userType msa", " ", NULL).string);
	}
	else
	{
		addToHPArray(&cmdargs, join("--uuid ", playerUUID, " ", NULL).string);
		addToHPArray(&cmdargs, join("--accessToken ", ".", " ", NULL).string);
		addToHPArray(&cmdargs, join("--userType legacy", " ", NULL).string);
	}
	addToHPArray(&cmdargs, join("--versionType release", NULL).string);

	return GetString(cmdargs);
}

