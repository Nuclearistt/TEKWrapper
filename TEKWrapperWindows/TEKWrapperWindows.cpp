#include <cstdio>
#include <Windows.h>

//Steam API type definitions
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
struct CCallbackBase
{
    virtual void Run(void* pvParam) = 0;
    virtual void Run(void* pvParam, bool bIOFailure, uint64 hSteamAPICall) = 0;
    virtual int GetCallbackSizeBytes() = 0;
    unsigned char m_nCallbackFlags;
    int m_iCallback;
};
struct ISteamGameServer {};
struct ISteamGameServerStats {};
struct ISteamNetworking {};
struct ISteamUtils {};
#pragma pack(push, 8)
struct GSClientApprove_t
{
    enum { k_iCallback = 201 };
    uint64 m_SteamID;
    uint64 m_OwnerSteamID;
};
#pragma pack(pop)
//Steam API function definitions
typedef void (*SteamAPI_RegisterCallback_t)(CCallbackBase*, int);
typedef void (*SteamAPI_UnregisterCallback_t)(CCallbackBase*);
typedef void (*SteamGameServer_RunCallbacks_t)();
typedef void (*SteamGameServer_Shutdown_t)();
typedef void (*SetKeyValue_t)(ISteamGameServer*, const char*, const char*);
typedef void (*SetGameData_t)(ISteamGameServer*, const char*);

//Global variables
HMODULE hModule; //Original Steam API DLL module handle
//Steam API interface pointers
ISteamGameServer* pSteamGameServer;
ISteamGameServerStats* pSteamGameServerStats;
ISteamNetworking* pSteamNetworking;
ISteamUtils* pSteamUtils;
//Steam API original function pointers
SteamAPI_RegisterCallback_t SteamAPI_RegisterCallback_o;
SteamAPI_UnregisterCallback_t SteamAPI_UnregisterCallback_o;
SteamGameServer_RunCallbacks_t SteamGameServer_RunCallbacks_o;
SteamGameServer_Shutdown_t SteamGameServer_Shutdown_o;
SetKeyValue_t SetKeyValue_o;
SetGameData_t SetGameData_o;
CCallbackBase* pClientApproveCallback; //Callback that needs to be executed when user login is approved
char Description[2048]{}; //TEK Wrapper's server description line returned in Steam server queries. The format is "TEKWrapper [ActiveMods] [InfoFileUrl]", if a string for [] is missing "0" is inserted instead

//Local functions
void SetKeyValue(ISteamGameServer* pThis, const char* pKey, const char* pValue)
{
    if (strcmp(pKey, "SEARCHKEYWORDS_s")) //SEARCHKEYWORDS_s is pretty much unused by ARK so it can be repurposed for TEK Wrapper needs
        SetKeyValue_o(pThis, pKey, pValue);
    else
        SetKeyValue_o(pThis, pKey, Description);
}
void SetGameData(ISteamGameServer* pThis, const char* pchGameData)
{
    const size_t cchGameData = strlen(pchGameData);
    char pchNewGameData[2048];
    memcpy(pchNewGameData, pchGameData, cchGameData);
    memcpy(pchNewGameData + cchGameData, ",TEKWrapper:1", 14); //This flag is required for server to be visible in TEK Launcher and game clients using TEK Injector
    SetGameData_o(pThis, pchNewGameData);
}
bool SendUserConnectAndAuthenticate(ISteamGameServer* pThis, uint32 unIPClient, const void* pvAuthBlob, uint32 cubAuthBlobSize, uint64* pSteamIDUser)
{
    if (cubAuthBlobSize < 20)
        return false;
    *pSteamIDUser = *((const uint64*)((const char*)pvAuthBlob + 12)); //The original function would fail for most users so we just get Steam ID from auth blob and send GSClientApprove callback with it
    GSClientApprove_t callback{ *pSteamIDUser, *pSteamIDUser };
    pClientApproveCallback->Run(&callback);
    return true;
}
int UserHasLicenseForApp() { return 0; } //0 means that user does have license
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) { return TRUE; }

//Functions that will be exported, matching signatures of all functions that the server imports from original steam_api64.dll
extern "C" __declspec(dllexport) void SteamAPI_RegisterCallback(CCallbackBase* pCallback, int iCallback)
{
    if (iCallback == 201) //Save GSClientApprove callback for further use
        pClientApproveCallback = pCallback;
    SteamAPI_RegisterCallback_o(pCallback, iCallback);
}
extern "C" __declspec(dllexport) void SteamAPI_UnregisterCallback(CCallbackBase* pCallback) { SteamAPI_UnregisterCallback_o(pCallback); }
extern "C" __declspec(dllexport) void* SteamApps() { return NULL; } //While client interface accessors are imported from the library they are unused so null can be returned here
extern "C" __declspec(dllexport) void* SteamFriends() { return NULL; }
extern "C" __declspec(dllexport) ISteamGameServer* SteamGameServer() { return pSteamGameServer; }
extern "C" __declspec(dllexport) ISteamNetworking* SteamGameServerNetworking() { return pSteamNetworking; }
extern "C" __declspec(dllexport) ISteamGameServerStats* SteamGameServerStats() { return pSteamGameServerStats; }
extern "C" __declspec(dllexport) ISteamUtils* SteamGameServerUtils() { return pSteamUtils; }
extern "C" __declspec(dllexport) bool SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, int eServerMode, const char* pchVersionString) //Since it's the first function called by server and it's called only once we can use it to initialize everything else
{
    //Retrieve command line first to search it for parameters
    char activeMods[1024]{ "0" };
    char infoFileUrl[256]{ "0" };
    int argc;
    const LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 0; i < argc; i++)
    {
        PCWSTR occurrence = wcsstr(argv[i], L"?GameModIds=");
        if (occurrence) //Parse mod list
        {
            occurrence += 12;
            PCWSTR occEnd = occurrence;
            for (; *occEnd && *occEnd != L'?'; ++occEnd);
            const int len = (const int)(occEnd - occurrence);
            WideCharToMultiByte(CP_UTF8, 0, occurrence, len, activeMods, len, NULL, NULL);
        }
        else if (occurrence = wcsstr(argv[i], L"-TWInfoFileUrl=")) //Load info file URL
        {
            occurrence += 15;
            const int len = (const int)wcslen(occurrence);
            WideCharToMultiByte(CP_UTF8, 0, occurrence, len, infoFileUrl, len, NULL, NULL);
            break;
        }
    }
    //Try to find active mods list in GameUserSettings.ini if it wasn't found in command line
    if (!strcmp(activeMods, "0"))
    {
        WCHAR pPath[MAX_PATH];
        int cchPath = GetCurrentDirectoryW(MAX_PATH, pPath);
        cchPath -= 14;
        memcpy(pPath + cchPath, L"Saved\\Config\\WindowsServer\\GameUserSettings.ini", 96);
        cchPath += 47;
        const HANDLE hFile = CreateFileW(pPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            return true;
        LARGE_INTEGER liSize;
        GetFileSizeEx(hFile, &liSize);
        const PSTR pFileBuffer = new char[liSize.QuadPart + 1];
        pFileBuffer[liSize.LowPart] = '\0';
        DWORD dwBytesRead;
        if (ReadFile(hFile, pFileBuffer, liSize.LowPart, &dwBytesRead, NULL))
        {
            PCSTR occurrence = strstr(pFileBuffer, "\nActiveMods=");
            if (occurrence)
            {
                occurrence += 12;
                PCSTR occEnd = occurrence;
                for (; *occEnd != '\r' && *occEnd != '\n'; ++occEnd);
                memcpy(activeMods, occurrence, (size_t)(occEnd - occurrence));
            }
        }
        delete[] pFileBuffer;
        CloseHandle(hFile);
    }
    snprintf(Description, 2048, "TEKWrapper %s %s", activeMods, infoFileUrl); //Create description string
    //Set Steam App ID to 346110
    SetEnvironmentVariableW(L"SteamAppId", L"346110");
    SetEnvironmentVariableW(L"GameAppId", L"346110");
    //Load steam_api64_o.dll, which is the original Steam API
    hModule = LoadLibraryW(L"steam_api64_o.dll");
    if (!hModule)
        return false;
    //Import functions from it that will be used outside of the scope of this function
    SteamAPI_RegisterCallback_o = (SteamAPI_RegisterCallback_t)GetProcAddress(hModule, "SteamAPI_RegisterCallback");
    SteamAPI_UnregisterCallback_o = (SteamAPI_UnregisterCallback_t)GetProcAddress(hModule, "SteamAPI_UnregisterCallback");
    SteamGameServer_RunCallbacks_o = (SteamGameServer_RunCallbacks_t)GetProcAddress(hModule, "SteamGameServer_RunCallbacks");
    SteamGameServer_Shutdown_o = (SteamGameServer_Shutdown_t)GetProcAddress(hModule, "SteamGameServer_Shutdown");
    //Call the original SteamGameServer_Init with the same parameters and retrieve interfaces from there
    if (!((bool(*)(uint32, uint16, uint16, uint16, int, const char*))GetProcAddress(hModule, "SteamGameServer_Init"))(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString))
        return false;
    pSteamGameServer = ((ISteamGameServer*(*)())GetProcAddress(hModule, "SteamGameServer"))();
    pSteamGameServerStats = ((ISteamGameServerStats*(*)())GetProcAddress(hModule, "SteamGameServerStats"))();
    pSteamNetworking = ((ISteamNetworking*(*)())GetProcAddress(hModule, "SteamGameServerNetworking"))();
    pSteamUtils = ((ISteamUtils*(*)())GetProcAddress(hModule, "SteamGameServerUtils"))();
    //Override some ISteamGameServer functions with my own using the same memory protection trick
    void** vfptr = *(void***)pSteamGameServer;
    //But make its virtual function table memory page writable first
    DWORD dwOldProtect;
    MEMORY_BASIC_INFORMATION mem;
    VirtualQuery(vfptr, &mem, sizeof(MEMORY_BASIC_INFORMATION));
    VirtualProtect(mem.BaseAddress, mem.RegionSize, PAGE_READWRITE, &dwOldProtect);
    SetKeyValue_o = (SetKeyValue_t)vfptr[20];
    SetGameData_o = (SetGameData_t)vfptr[22];
    vfptr[20] = SetKeyValue;
    vfptr[22] = SetGameData;
    vfptr[24] = SendUserConnectAndAuthenticate;
    vfptr[32] = UserHasLicenseForApp;
    VirtualProtect(mem.BaseAddress, mem.RegionSize, dwOldProtect, &dwOldProtect); //Return old protection just in case
    return true;
}
extern "C" __declspec(dllexport) void SteamGameServer_RunCallbacks() { SteamGameServer_RunCallbacks_o(); }
extern "C" __declspec(dllexport) void SteamGameServer_Shutdown()
{
    SteamGameServer_Shutdown_o();
    FreeLibrary(hModule);
}
extern "C" __declspec(dllexport) void* SteamInventory() { return NULL; }
extern "C" __declspec(dllexport) void* SteamMatchmaking() { return NULL; }
extern "C" __declspec(dllexport) void* SteamMatchmakingServers() { return NULL; }
extern "C" __declspec(dllexport) void* SteamNetworking() { return NULL; }
extern "C" __declspec(dllexport) void* SteamRemoteStorage() { return NULL; }
extern "C" __declspec(dllexport) void* SteamUGC() { return NULL; }
extern "C" __declspec(dllexport) void* SteamUser() { return NULL; }
extern "C" __declspec(dllexport) void* SteamUserStats() { return NULL; }
extern "C" __declspec(dllexport) void* SteamUtils() { return NULL; }