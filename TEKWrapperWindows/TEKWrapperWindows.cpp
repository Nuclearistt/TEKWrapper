#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0601 //Win7 compatibility
#include <SDKDDKVer.h>
//Don't include unnecessary APIs
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#include <Windows.h>
#include <cwchar>

//Util function
template<typename T> constexpr inline T* PtrOffset(_In_ PVOID address, _In_ DWORD offset)
{
	return reinterpret_cast<T*>(reinterpret_cast<BYTE*>(address) + offset);
}

//Steam API type definitions
struct SteamInterface //Generic representation of a C++ interface
{
	const void* const* VirtualMethodTable;
};
struct CallbackBase
{
	virtual void Run(PVOID parameter) = 0;
	virtual void Run(PVOID parameter, bool ioFailure, DWORD64 apiCallHandle) = 0;
	virtual DWORD GetCallbackSizeBytes() = 0;
	BYTE CallbackFlags;
	int CallbackIndex;
};
struct ClientApproveCallback
{
	DWORD64 SteamId;
	DWORD64 OwnerSteamId;
};

//Steam API function imports
extern "C"
{
	__declspec(dllimport) void SteamAPI_RegisterCallback(_In_ CallbackBase* callback, _In_ DWORD callbackIndex);
	__declspec(dllimport) bool SteamGameServer_Init(_In_ DWORD ip, _In_ WORD steamPort, _In_ WORD gamePort, _In_ WORD queryPort, _In_ int serverMode, _In_ PCSTR versionString);
}

//Wrapper type definitions
struct SteamInterfaceWrapper
{
	const void* const* VirtualMethodTable; //Redirects to SteamApiInterface all functions that are not overridden
	SteamInterface* SteamApiInterface; //Wrapped interface pointer
	const void* VirtualMethodTableData[44]; //Every element should point to the according entry in RedirectFunctions unless overridden; 44 is the number of methods in version 012 of ISteamGameServer
#pragma code_seg(push, ".text")
	//Contains 44 16-byte blocks of the following code:
	//	mov rcx, qword ptr [rcx+8] ;Set SteamApiInterface as the first parameter
	//	mov rax, qword ptr [rcx] ;Load original interace's virtual method table
	//	jmp qword ptr [rax+n] ;n is the index of method in the table multiplied by 8
	//	int3 padding until the end of 16-byte block
	__declspec(allocate(".text")) constexpr static BYTE RedirectFunctions[0x2C0]
	{
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x20, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x08, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x10, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x18, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x20, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x28, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x30, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x38, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x40, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x48, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x50, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x58, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x60, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x68, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x70, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0x60, 0x78, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x80, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x88, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x90, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x98, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xA8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xB0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xB8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xC0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xC8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xD0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xD8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xE0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xE8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xF0, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0xF8, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x00, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x08, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x10, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x18, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x20, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x28, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x30, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x38, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x40, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x48, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x50, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
		0x48, 0x8B, 0x49, 0x08, 0x48, 0x8B, 0x01, 0xFF, 0xA0, 0x58, 0x01, 0x00, 0x00, 0xCC, 0xCC, 0xCC
	};
#pragma code_seg(pop)
	constexpr inline void Initialize(_In_ SteamInterface* originalInterface)
	{
		SteamApiInterface = originalInterface;
		VirtualMethodTable = VirtualMethodTableData;
		for (DWORD i = 0; i < 44; ++i)
			VirtualMethodTableData[i] = RedirectFunctions + i * 0x10;
	}
};

//Globals
PSTR Description; //TEK Wrapper's server description line returned in Steam server queries. The format is "TEKWrapper {ActiveMods} {InfoFileUrl}", if {ActiveMods} or {InfoFileUrl} are missing they are replaced with "0"
SteamInterfaceWrapper SteamGameServerWrapper;
CallbackBase* ApproveCallback;

//Steam API replacement functions
void SetKeyValue(_In_ SteamInterfaceWrapper* iSteamGameServerWrapper, _In_ PCSTR key, _In_ PCSTR value)
{
	const DWORD64* const key64 = reinterpret_cast<const DWORD64*>(key); //Merely a reinterpretation to allow comparing 8 characters of key in one instruction
	if (strlen(key) == 16 && key64[0] == 0x454B484352414553 && key64[1] == 0x735F5344524F5759) //~ key == "SEARCHKEYWORDS_s", this key is pretty much unused by ARK so it can be repurposed for TEK Wrapper's needs
		reinterpret_cast<void(*)(SteamInterface*, PCSTR, PCSTR)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[20])(iSteamGameServerWrapper->SteamApiInterface, key, Description); //Forward call to original method with value set to Description
	else
		reinterpret_cast<void(*)(SteamInterface*, PCSTR, PCSTR)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[20])(iSteamGameServerWrapper->SteamApiInterface, key, value); //Forward call to original method with same parameters
}
void SetGameData(_In_ SteamInterfaceWrapper* iSteamGameServerWrapper, _In_ PCSTR gameData)
{
	const HANDLE processHeap = GetProcessHeap();
	const size_t gameDataSize = strlen(gameData);
	const PSTR newGameData = reinterpret_cast<PSTR>(HeapAlloc(processHeap, 0, gameDataSize + 14));
	if (newGameData)
	{
		memcpy(newGameData, gameData, gameDataSize);
		memcpy(newGameData + gameDataSize, ",TEKWrapper:1", 14); //This flag is required for server to be visible in TEK Launcher and game clients that use ARK Shellcode
		reinterpret_cast<void(*)(SteamInterface*, PCSTR)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[22])(iSteamGameServerWrapper->SteamApiInterface, newGameData);
		HeapFree(processHeap, 0, newGameData);
	}
	else
		reinterpret_cast<void(*)(SteamInterface*, PCSTR)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[22])(iSteamGameServerWrapper->SteamApiInterface, gameData);
}
#pragma warning (suppress: 6101)
bool SendUserConnectAndAuthenticate(_In_ SteamInterfaceWrapper* iSteamGameServerWrapper, _In_ DWORD clientIp, _In_reads_bytes_(authBlobSize) const void* authBlob, _In_ DWORD authBlobSize, _Out_ DWORD64& steamId)
{
	if (authBlobSize < 20)
		return false;
	if (!reinterpret_cast<bool(*)(SteamInterface*, DWORD, const void*, DWORD, DWORD64&)>(iSteamGameServerWrapper->SteamApiInterface->VirtualMethodTable[24])(iSteamGameServerWrapper->SteamApiInterface, clientIp, authBlob, authBlobSize, steamId))
	{
		//Steam denied connection, but we won't
		steamId = *reinterpret_cast<const DWORD64*>(reinterpret_cast<const BYTE*>(authBlob) + 12);
		ClientApproveCallback callback { steamId, steamId };
		ApproveCallback->Run(&callback);
	}
	return true;
}
DWORD UserHasLicenseForApp() { return 0; } //0 means that user does have license
extern "C"
{
	void SteamAPI_RegisterCallbackWrapper(_In_ CallbackBase* callback, _In_ DWORD callbackIndex)
	{
		if (callbackIndex == 201)
			ApproveCallback = callback; //Save it for further use
		if (callbackIndex != 202) //Deny denial callback
			SteamAPI_RegisterCallback(callback, callbackIndex);
	}
	bool SteamGameServer_InitWrapper(_In_ DWORD ip, _In_ WORD steamPort, _In_ WORD gamePort, _In_ WORD queryPort, _In_ int serverMode, _In_ PCSTR versionString)
	{
		const HANDLE processHeap = GetProcessHeap();
		PCWSTR activeMods = L"0";
		int activeModsLength = 1;
		PCWSTR infoFileUrl = L"0";
		int infoFileUrlLength = 1;
		bool activeModsAllocated = false;
		//Search command line for parameters
		const LPCWSTR commandLine = GetCommandLineW();
		PCWSTR occurrence = wcsstr(commandLine, L"-TWInfoFileUrl=");
		if (occurrence)
		{
			infoFileUrl = occurrence + 15;
			PCWSTR occEnd = infoFileUrl;
			for (; *occEnd && *occEnd != L' '; ++occEnd);
			infoFileUrlLength = static_cast<int>(occEnd - infoFileUrl);
		}
		if (occurrence = wcsstr(commandLine, L"?GameModIds="))
		{
			activeMods = occurrence + 12;
			PCWSTR occEnd = activeMods;
			for (; *occEnd && *occEnd != L' ' && *occEnd != L'?'; ++occEnd);
			activeModsLength = static_cast<int>(occEnd - activeMods);
		}
		else //Search GameUserSettings.ini for ActiveMods
		{
			WCHAR filePath[MAX_PATH];
			memcpy(filePath + GetCurrentDirectoryW(MAX_PATH, filePath) - 14, L"Saved\\Config\\WindowsServer\\GameUserSettings.ini", 96);
			const HANDLE file = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (file != INVALID_HANDLE_VALUE)
			{
				LARGE_INTEGER fileSize;
				GetFileSizeEx(file, &fileSize);
				const PSTR fileBuffer = reinterpret_cast<PSTR>(HeapAlloc(processHeap, 0, fileSize.QuadPart + 1));
				if (fileBuffer)
				{
					fileBuffer[fileSize.LowPart] = '\0';
					DWORD bytesRead;
					if (ReadFile(file, fileBuffer, fileSize.LowPart, &bytesRead, NULL))
					{
						PCSTR fileOccurrence = strstr(fileBuffer, "\nActiveMods=");
						if (fileOccurrence)
						{
							fileOccurrence += 12;
							PCSTR fileOccEnd = fileOccurrence;
							for (; *fileOccEnd && *fileOccEnd != '\r' && *fileOccEnd != '\n'; ++fileOccEnd);
							const int utf8Length = static_cast<int>(fileOccEnd - fileOccurrence);
							const int unicodeLength = MultiByteToWideChar(CP_UTF8, 0, fileOccurrence, utf8Length, NULL, 0);
							if (unicodeLength)
							{
								const PWSTR activeModsBuffer = reinterpret_cast<PWSTR>(HeapAlloc(processHeap, 0, unicodeLength * sizeof(WCHAR)));
								if (activeModsBuffer)
								{
									MultiByteToWideChar(CP_UTF8, 0, fileOccurrence, utf8Length, activeModsBuffer, unicodeLength);
									activeMods = activeModsBuffer;
									activeModsLength = unicodeLength;
									activeModsAllocated = true;
								}
							}
						}
					}
					HeapFree(processHeap, 0, fileBuffer);
				}
				CloseHandle(file);
			}
		}
		//Create description string
		const int activeModsUtf8Length = WideCharToMultiByte(CP_UTF8, 0, activeMods, activeModsLength, NULL, 0, NULL, NULL);
		const int infoFileUrlUtf8Length = WideCharToMultiByte(CP_UTF8, 0, infoFileUrl, infoFileUrlLength, NULL, 0, NULL, NULL);
		const SIZE_T descriptionBufferLength = static_cast<SIZE_T>(13 + activeModsUtf8Length + infoFileUrlUtf8Length);
		const PSTR descriptionBuffer = reinterpret_cast<PSTR>(HeapAlloc(processHeap, 0, descriptionBufferLength));
		if (!descriptionBuffer)
			return false;
		descriptionBuffer[descriptionBufferLength - 1] = '\0';
#pragma warning (suppress: 6386)
		descriptionBuffer[11 + activeModsUtf8Length] = ' ';
		memcpy(descriptionBuffer, "TEKWrapper ", 11);
		WideCharToMultiByte(CP_UTF8, 0, activeMods, activeModsLength, descriptionBuffer + 11, activeModsUtf8Length, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, infoFileUrl, infoFileUrlLength, descriptionBuffer + 12 + activeModsUtf8Length, infoFileUrlUtf8Length, NULL, NULL);
		if (activeModsAllocated)
			HeapFree(processHeap, 0, const_cast<PWSTR>(activeMods));
		Description = descriptionBuffer;
		//Set Steam app ID to 346110
		SetEnvironmentVariableW(L"SteamAppId", L"346110");
		SetEnvironmentVariableW(L"GameAppId", L"346110");
		//Call original SteamGameServer_Init to initialize interfaces
		if (!SteamGameServer_Init(ip, steamPort, gamePort, queryPort, serverMode, versionString))
			return false;
		//Wrap ISteamGameServer interface and redirect methods
		SteamGameServerWrapper.Initialize(*PtrOffset<SteamInterface*>(SteamGameServer_Init, 0x29FE8)); //0x29FE8 is offset from beginning of SteamGameServer_Init function to Steam API's internal variable that holds ISteamGameServer pointer. ARK never updates steam_api64.dll so such trick is working without risks
		*PtrOffset<SteamInterfaceWrapper*>(SteamGameServer_Init, 0x29FE8) = &SteamGameServerWrapper;
		SteamGameServerWrapper.VirtualMethodTableData[20] = SetKeyValue;
		SteamGameServerWrapper.VirtualMethodTableData[22] = SetGameData;
		SteamGameServerWrapper.VirtualMethodTableData[24] = SendUserConnectAndAuthenticate;
		SteamGameServerWrapper.VirtualMethodTableData[32] = UserHasLicenseForApp;
		return true;
	}
}

//DLL entry point
BOOL Main(_In_ HMODULE baseAddress, _In_ DWORD callReason)
{
	if (callReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(baseAddress);
	return TRUE;
}