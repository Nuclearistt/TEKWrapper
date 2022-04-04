#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/mman.h>
#include <sys/stat.h>

//Steam API type definitions
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;
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
void* dlHandle; //Original Steam API dynamic library handle
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

//Functions that will be exported, matching signatures of all functions that the server imports from original libsteam_api.so
extern "C" void SteamAPI_RegisterCallback(CCallbackBase* pCallback, int iCallback)
{
    if (iCallback == 201) //Save GSClientApprove callback for further use
        pClientApproveCallback = pCallback;
    SteamAPI_RegisterCallback_o(pCallback, iCallback);
}
extern "C" void SteamAPI_UnregisterCallback(CCallbackBase* pCallback) { SteamAPI_UnregisterCallback_o(pCallback); }
extern "C" void* SteamApps() { return nullptr; } //While client interface accessors are imported from the library they are unused so null can be returned here
extern "C" void* SteamFriends() { return nullptr; }
extern "C" ISteamGameServer* SteamGameServer() { return pSteamGameServer; }
extern "C" ISteamNetworking* SteamGameServerNetworking() { return pSteamNetworking; }
extern "C" ISteamGameServerStats* SteamGameServerStats() { return pSteamGameServerStats; }
extern "C" ISteamUtils* SteamGameServerUtils() { return pSteamUtils; }
extern "C" bool SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, int eServerMode, const char* pchVersionString) //Since it's the first function called by server and it's called only once we can use it to initialize everything else
{
    //Retrieve command line first to search it for parameters
    char buffer[4096]{};
    char activeMods[1024]{ "0" };
    char infoFileUrl[256]{ "0" };
    int fd = open("/proc/self/cmdline", O_RDONLY);
    const char* const end = buffer + read(fd, buffer, 4096);
    close(fd);
    for (const char* i = buffer; i < end;)
    {
        const char* occurrence = strstr(i, "?GameModIds=");
        if (occurrence) //Parse mod list
        {
            occurrence += 12;
            const char* occEnd = occurrence;
            for (; *occEnd && *occEnd != '?'; ++occEnd);
            memcpy(activeMods, occurrence, (size_t)(occEnd - occurrence));
        }
        else if (occurrence = strstr(i, "-TWInfoFileUrl=")) //Load info file URL
        {
            occurrence += 15;
            memcpy(infoFileUrl, occurrence, strlen(occurrence));
            break;
        }
        while (*i++); // Go to the next null-terminated section
    }
    //Try to find active mods list in GameUserSettings.ini if it wasn't found in command line
    if (!strcmp(activeMods, "0"))
    {
        char pPath[PATH_MAX];
        getcwd(pPath, PATH_MAX);
        memcpy(pPath + strlen(pPath) - 14, "Saved/Config/LinuxServer/GameUserSettings.ini", 46);
        struct stat statbuf;
        if (stat(pPath, &statbuf) != -1)
        {
            if ((fd = open(pPath, O_RDONLY)) != -1)
            {
                char* const pFileBuffer = new char[statbuf.st_size + 1];
                pFileBuffer[statbuf.st_size] = '\0';
                if (read(fd, pFileBuffer, statbuf.st_size) > 0)
                {
                    const char* occurrence = strstr(pFileBuffer, "\nActiveMods=");
                    if (occurrence)
                    {
                        occurrence += 12;
                        const char* occEnd = occurrence;
                        for (; *occEnd != '\r' && *occEnd != '\n'; ++occEnd);
                        memcpy(activeMods, occurrence, (size_t)(occEnd - occurrence));
                    }
                }
                delete[] pFileBuffer;
                close(fd);
            }
        }
    }
    snprintf(Description, 2048, "TEKWrapper %s %s", activeMods, infoFileUrl); //Create description string
    //Set Steam App ID to 346110
    setenv("SteamAppId", "346110", 1);
    setenv("GameAppId", "346110", 1);
    //Load libsteam_api_o.so, which is the original Steam API
    dlHandle = dlopen("libsteam_api_o.so", RTLD_LAZY);
    if (!dlHandle)
        return false;
    //Import functions from it that will be used outside of the scope of this function
    SteamAPI_RegisterCallback_o = (SteamAPI_RegisterCallback_t)dlsym(dlHandle, "SteamAPI_RegisterCallback");
    SteamAPI_UnregisterCallback_o = (SteamAPI_UnregisterCallback_t)dlsym(dlHandle, "SteamAPI_UnregisterCallback");
    SteamGameServer_RunCallbacks_o = (SteamGameServer_RunCallbacks_t)dlsym(dlHandle, "SteamGameServer_RunCallbacks");
    SteamGameServer_Shutdown_o = (SteamGameServer_Shutdown_t)dlsym(dlHandle, "SteamGameServer_Shutdown");
    //Call the original SteamGameServer_Init with the same parameters and retrieve interfaces from there
    if (!((bool(*)(uint32, uint16, uint16, uint16, int, const char*))dlsym(dlHandle, "SteamGameServer_Init"))(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString))
        return false;
    pSteamGameServer = ((ISteamGameServer*(*)())dlsym(dlHandle, "SteamGameServer"))();
    pSteamGameServerStats = ((ISteamGameServerStats*(*)())dlsym(dlHandle, "SteamGameServerStats"))();
    pSteamNetworking = ((ISteamNetworking*(*)())dlsym(dlHandle, "SteamGameServerNetworking"))();
    pSteamUtils = ((ISteamUtils*(*)())dlsym(dlHandle, "SteamGameServerUtils"))();
    //Override some ISteamGameServer functions with my own
    void** vfptr = *(void***)pSteamGameServer;
    //But make its virtual function table memory page writable first
    const long pageSize = sysconf(_SC_PAGESIZE); //Get page size so we can compute base address of the page that needs to be modifed
    mprotect((void*)((long)vfptr & (~(pageSize - 1))), pageSize, PROT_READ | PROT_WRITE | PROT_EXEC); //And change page's protection
    SetKeyValue_o = (SetKeyValue_t)vfptr[20];
    SetGameData_o = (SetGameData_t)vfptr[22];
    vfptr[20] = (void*)SetKeyValue;
    vfptr[22] = (void*)SetGameData;
    vfptr[24] = (void*)SendUserConnectAndAuthenticate;
    vfptr[32] = (void*)UserHasLicenseForApp;
    return true;
}
extern "C" void SteamGameServer_RunCallbacks() { SteamGameServer_RunCallbacks_o(); }
extern "C" void SteamGameServer_Shutdown()
{
    SteamGameServer_Shutdown_o();
    dlclose(dlHandle);
}
extern "C" void* SteamInventory() { return nullptr; }
extern "C" void* SteamMatchmaking() { return nullptr; }
extern "C" void* SteamMatchmakingServers() { return nullptr; }
extern "C" void* SteamNetworking() { return nullptr; }
extern "C" void* SteamRemoteStorage() { return nullptr; }
extern "C" void* SteamUGC() { return nullptr; }
extern "C" void* SteamUser() { return nullptr; }
extern "C" void* SteamUserStats() { return nullptr; }
extern "C" void* SteamUtils() { return nullptr; }