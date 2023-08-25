#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

//Basic type definitions for convenience
typedef unsigned char byte;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;
typedef unsigned long long uint64;
typedef void* ptr;
typedef const void* cptr;
typedef char* str;
typedef const char* cstr;

//Pointer util functions
template<typename T> constexpr inline T* PtrOffset(ptr base, uint32 offset)
{
	return reinterpret_cast<T*>(reinterpret_cast<byte*>(base) + offset);
}
template<typename T> constexpr inline const T* PtrOffset(cptr base, uint32 offset)
{
	return reinterpret_cast<const T*>(reinterpret_cast<const byte*>(base) + offset);
}

//Steam API type definitions
struct SteamInterface //Generic representation of a C++ interface
{
	cptr* VirtualMethodTable;
};
struct CallbackBase
{
	virtual void Run(ptr parameter) = 0;
	virtual void Run(ptr parameter, bool ioFailure, uint64 apiCallHandle) = 0;
	virtual int32 GetCallbackSizeBytes() = 0;
	byte CallbackFlags;
	int CallbackIndex;
};
struct ClientApproveCallback
{
	uint64 SteamId;
	uint64 OwnerSteamId;
};

//Wrapper type definitions
struct SteamInterfaceWrapper
{
	cptr* VirtualMethodTable; //Redirects to SteamApiInterface all functions that are not overridden
	SteamInterface* SteamApiInterface; //Wrapped interface pointer
	cptr VirtualMethodTableData[44]; //Every element should point to the according entry in RedirectFunctions unless overridden; 44 is the number of methods in version 012 of ISteamGameServer
	//Contains 44 16-byte blocks of the following code:
	//	mov rdi, qword ptr [rdi+8] ;Set SteamApiInterface as the first parameter
	//	mov rax, qword ptr [rdi] ;Load original interace's virtual method table
	//	jmp qword ptr [rax+n] ;n is the index of method in the table multiplied by 8
	//	int3 padding until the end of 16-byte block
	constexpr static byte RedirectFunctions[0x2C0] __attribute__((section(".text")))
	{
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x20, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x08, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x10, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x18, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x20, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x28, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x30, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x38, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x40, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x48, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x50, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x58, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x60, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x68, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x70, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0x60, 0x78, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x80, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x88, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x90, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x98, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xA8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xB0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xB8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xC0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xC8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xD0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xD8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xE0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xE8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xF0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0xF8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x00, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x08, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x10, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x18, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x20, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x28, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x30, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x38, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x40, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x48, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x50, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x7F, 0x08, 0x48, 0x8B, 0x07, 0xFF, 0xA0, 0x58, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC
	};
	constexpr inline void Initialize(SteamInterface* originalInterface)
	{
		SteamApiInterface = originalInterface;
		VirtualMethodTable = VirtualMethodTableData;
		for (uint32 i = 0; i < 44; ++i)
			VirtualMethodTableData[i] = RedirectFunctions + i * 0x10;
	}
};

//Globals
void(*SteamAPI_RegisterCallback_o)(CallbackBase*, uint32); //Address of original SteamAPI_RegisterCallback obtained via dlsym
str Description; //TEK Wrapper's server description line returned in Steam server queries. The format is "TEKWrapper {ActiveMods} {InfoFileUrl}", if {ActiveMods} or {InfoFileUrl} are missing they are replaced with "0"
SteamInterfaceWrapper SteamGameServerWrapper;
CallbackBase* ApproveCallback;

//Steam API replacement functions
#pragma GCC visibility push(hidden)
void SetKeyValue(SteamInterfaceWrapper* iSteamGameServerWrapper, cstr key, cstr value)
{
	const uint64* const key64 = reinterpret_cast<const uint64*>(key); //Merely a reinterpretation to allow comparing 8 characters of key in one instruction
	if (strlen(key) == 16 && key64[0] == 0x454B484352414553 && key64[1] == 0x735F5344524F5759) //~ key == "SEARCHKEYWORDS_s", this key is pretty much unused by ARK so it can be repurposed for TEK Wrapper's needs
		reinterpret_cast<void(*)(SteamInterface*, cstr, cstr)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[20])(iSteamGameServerWrapper->SteamApiInterface, key, Description); //Forward call to original method with value set to Description
	else
		reinterpret_cast<void(*)(SteamInterface*, cstr, cstr)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[20])(iSteamGameServerWrapper->SteamApiInterface, key, value); //Forward call to original method with same parameters
}
void SetGameData(SteamInterfaceWrapper* iSteamGameServerWrapper, cstr gameData)
{
	const size_t gameDataSize = strlen(gameData);
	const str newGameData = new char[gameDataSize + 14];
	memcpy(newGameData, gameData, gameDataSize);
	memcpy(newGameData + gameDataSize, ",TEKWrapper:1", 14); //This flag is required for server to be visible in TEK Launcher and game clients that use ARK Shellcode
	reinterpret_cast<void(*)(SteamInterface*, cstr)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[22])(iSteamGameServerWrapper->SteamApiInterface, newGameData);
	delete[] newGameData;
}
bool SendUserConnectAndAuthenticate(SteamInterfaceWrapper* iSteamGameServerWrapper, uint32 clientIp, cptr authBlob, uint32 authBlobSize, uint64& steamId)
{
	if (authBlobSize < 20)
		return false;
	if (!reinterpret_cast<bool(*)(SteamInterface*, uint32, cptr, uint32, uint64&)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[24])(iSteamGameServerWrapper->SteamApiInterface, clientIp, authBlob, authBlobSize, steamId))
	{
		//Steam denied connection, but we won't
		steamId = *PtrOffset<uint64>(authBlob, 12);
		ClientApproveCallback callback { steamId, steamId };
		ApproveCallback->Run(&callback);
	}
	return true;
}
int32 UserHasLicenseForApp() { return 0; } //0 means that user does have license
#pragma GCC visibility pop
extern "C"
{
	void SteamAPI_RegisterCallback(CallbackBase* callback, uint32 callbackIndex)
	{
		if (callbackIndex == 201)
			ApproveCallback = callback; //Save it for further use
		if (callbackIndex != 202) //Deny denial callback
			SteamAPI_RegisterCallback_o(callback, callbackIndex);
	}
	bool SteamGameServer_Init(uint32 ip, uint16 steamPort, uint16 gamePort, uint16 queryPort, int serverMode, cstr versionString)
	{
		cstr activeMods = new char[1] { '0' };
		size_t activeModsSize = 1;
		cstr infoFileUrl = new char[1] { '0' };
		size_t infoFileUrlSize = 1;
		str buffer = new char[0x1000];
		//Search command line for parameters
		int fd = open("/proc/self/cmdline", O_RDONLY);
		const cstr end = buffer + read(fd, buffer, 0x1000);
		close(fd);
		for (cstr i = buffer; i < end;)
		{
		    cstr occurrence = strstr(i, "?GameModIds=");
		    if (occurrence)
		    {
				occurrence += 12;
		        cstr occEnd = occurrence;
		        for (; *occEnd && *occEnd != '?'; ++occEnd) {}
				activeModsSize = occEnd - occurrence;
				str activeModsBuffer = new char[activeModsSize];
				memcpy(activeModsBuffer, occurrence, activeModsSize);
				delete[] activeMods;
				activeMods = activeModsBuffer;
		    }
		    else if ((occurrence = strstr(i, "-TWInfoFileUrl="))) //Load info file URL
		    {
				occurrence += 15;
				infoFileUrlSize = strlen(occurrence);
				str infoFileUrlBuffer = new char[infoFileUrlSize];
				memcpy(infoFileUrlBuffer, occurrence, infoFileUrlSize);
				delete[] infoFileUrl;
				infoFileUrl = infoFileUrlBuffer;
		        break;
		    }
		    while (*i++); // Go to the next null-terminated section
		}
		if (activeModsSize == 1) //Search GameUserSettings.ini for ActiveMods
		{
		    if (!getcwd(buffer, 0x1000)) {}
		    memcpy(buffer + strlen(buffer) - 14, "Saved/Config/LinuxServer/GameUserSettings.ini", 46);
		    struct stat statbuf;
		    if (stat(buffer, &statbuf) >= 0 && (fd = open(buffer, O_RDONLY)) >= 0)
		    {
				str fileBuffer = new char[statbuf.st_size + 1];
				fileBuffer[statbuf.st_size] = '\0';
				if (read(fd, fileBuffer, statbuf.st_size) > 0)
				{
					cstr occurrence = strstr(fileBuffer, "\nActiveMods=");
					if (occurrence)
					{
						occurrence += 12;
						cstr occEnd = occurrence;
						for (; *occEnd && *occEnd != '\r' && *occEnd != '\n'; ++occEnd) {}
						activeModsSize = occEnd - occurrence;
						str activeModsBuffer = new char[activeModsSize];
						memcpy(activeModsBuffer, occurrence, activeModsSize);
						delete[] activeMods;
						activeMods = activeModsBuffer;
					}
				}
				delete[] fileBuffer;
				close(fd);
		    }
		}
		delete[] buffer;
		//Create description string
		const size_t descriptionBufferSize = 13 + activeModsSize + infoFileUrlSize;
		const str descriptionBuffer = new char[descriptionBufferSize];
		descriptionBuffer[descriptionBufferSize - 1] = '\0';
		descriptionBuffer[11 + activeModsSize] = ' ';
		memcpy(descriptionBuffer, "TEKWrapper ", 11);
		memcpy(descriptionBuffer + 11, activeMods, activeModsSize);
		memcpy(descriptionBuffer + 12 + activeModsSize, infoFileUrl, infoFileUrlSize);
		delete[] activeMods;
		delete[] infoFileUrl;
		Description = descriptionBuffer;
		//Set Steam app ID to 346110
		setenv("SteamAppId", "346110", 1);
		setenv("GameAppId", "346110", 1);
		//Open libsteam_api_o.so to load original function addresses from it
		const ptr dlHandle = dlopen("libsteam_api_o.so", RTLD_LAZY | RTLD_GLOBAL);
		SteamAPI_RegisterCallback_o = reinterpret_cast<void(*)(CallbackBase*, uint32)>(dlsym(dlHandle, "SteamAPI_RegisterCallback"));
		ptr steamGameServer_Init = dlsym(dlHandle, "SteamGameServer_Init");
		//Call original SteamGameServer_Init to initialize interfaces
		const bool initSucceeded = reinterpret_cast<bool(*)(uint32, uint16, uint16, uint16, int, cstr)>(steamGameServer_Init)(ip, steamPort, gamePort, queryPort, serverMode, versionString);
		if (!initSucceeded)
			return false;
		//Wrap ISteamGameServer interface and redirect methods
		SteamGameServerWrapper.Initialize(*PtrOffset<SteamInterface*>(steamGameServer_Init, 0x209384)); //0x209384 is offset from beginning of SteamGameServer_Init function to Steam API's internal variable that holds ISteamGameServer pointer. ARK never updates libsteam_api.so so such trick is working without risks
		*PtrOffset<SteamInterfaceWrapper*>(steamGameServer_Init, 0x209384) = &SteamGameServerWrapper;
		SteamGameServerWrapper.VirtualMethodTableData[20] = reinterpret_cast<cptr>(SetKeyValue);
		SteamGameServerWrapper.VirtualMethodTableData[22] = reinterpret_cast<cptr>(SetGameData);
		SteamGameServerWrapper.VirtualMethodTableData[24] = reinterpret_cast<cptr>(SendUserConnectAndAuthenticate);
		SteamGameServerWrapper.VirtualMethodTableData[32] = reinterpret_cast<cptr>(UserHasLicenseForApp);
		return true;
	}
}