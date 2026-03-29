# TEK Wrapper
[![Discord](https://img.shields.io/discord/937821572285206659?style=flat-square&label=Discord&logo=discord&logoColor=white&color=7289DA)](https://discord.gg/JBUgcwvpfc)

## Overview

TEK Wrapper is a library for ARK: Survival Evolved dedicated server that allows users that don't own the game and/or DLCs to join by ignoring the result of Steam's ownership checks, and also modifies Steam server rules query response to include additional information about the server, which consists of the mark to identify that TEK Wrapper is used, active mods list and optionally a URL to file with extra server description that can be displayed in [TEK Launcher](https://github.com/Nuclearistt/TEKLauncher)

**Notes for hosting service provider users:**
- Setting up TEK Wrapper is only possible if the service provides you with sufficient access to server's files and folders. If you can't find some of the files/folders mentioned in the guide or are unable to rename/replace them, then unfortunately, you will not be able to use it.
- Keep in mind that hosting serices may be using either Windows or Linux as the OS, and most of them don't specify that info. If you're unsure, do installation steps for both operating systems, they don't contradict each other.
- Even if you manage to set up the files with no issues, some services perform file validation on server startup via steamcmd, which overwrites TEK Wrapper files with original ones. Usually it's part of the auto-update procedure, and your service may have an option called something like "Disable Auto Updates" to disable that

## How to use

1. Set up a dedicated server. If you're not sure how to do it, the wiki has a [tutorial](https://ark.wiki.gg/wiki/Dedicated_server_setup) for that. You can use tools like ARK Server Manager (ASM) too, as well as *some* hosting service providers (see notes above for those). **Do not** install TEK Wrapper on your client installation (the game that you play), it may cause conflicts with client-side software
2. (**{Server root}** below is placeholder for your dedicated server installation folder)   
  **For Windows-run servers**: 
   - Go to **{Server root}\Engine\Binaries\ThirdParty\Steamworks\Steamv132\Win64** folder, there rename **steam_api64.dll** to **steam_api64_o.dll** and move it to **{Server root}\ShooterGame\Binaries\Win64** folder
   - Download the latest released TEK Wrapper library file [here](https://github.com/Nuclearistt/TEKWrapper/releases/latest/download/steam_api64.dll) and put it into **{Server root}\Engine\Binaries\ThirdParty\Steamworks\Steamv132\Win64** folder

   **For Linux-run servers**:
   - Go to **{Server root}/ShooterGame/Binaries/Linux** folder, there rename **libsteam_api.so** to **libsteam_api_o.so**
   - Download the latest released TEK Wrapper library file [here](https://github.com/Nuclearistt/TEKWrapper/releases/latest/download/libsteam_api.so) and put it into **{Server root}/ShooterGame/Binaries/Linux** folder
4. Make sure that BattlEye is disabled on the server, otherwise players without the license won't be able to join it. Moreover, TEK Launcher adds a search filter that prevents BattlEye-protected servers from being displayed in any lists
5. (Optional) If you want to provide extra information about your server/cluster to be displayed in TEK Launcher, add *-TWInfoFileUrl=url* to its command-line arguments (for example: *-TWInfoFileUrl=https://example.com/Info.json*). The URL must point to a valid json file or **direct** download link for one, that can be accessed by any outside user. The format of the file is described in next section. If you host a cluster, it's recommended to use the same file URL for all servers of the cluster if they have the same description and naming policy; TEK Launcher takes advantage of that by caching the file so it doesn't have to be downloaded multiple times

## Server info file format

Example:
```json
{
  "HosterName": "Nuclearist",
  "ServerName": "My server",
  "ClusterName": "My cluster",
  "IconUrl": "https://example.com/Icon.png",
  "Discord": "https://discord.gg/47SFqqMBFN",
  "ServerDescription":
  {
    "MaxDinoLvl": 180,
    "Taming": 2,
    "Experience": 1,
    "Harvesting": 6,
    "Breeding": 1,
    "Stacks": 1,
    "Other":
    [
        "Some feature specific to this very server"
    ]
  },
  "ClusterDescription":
  {
    "MaxDinoLvl": 150,
    "Taming": 1,
    "Experience": 1,
    "Harvesting": 1,
    "Breeding": 1,
    "Stacks": 1,
    "Other":
    [
        "Some extra feature of my cluster",
        "And another one"
    ]
  }
}
```
Every property in the object is optional, so you can remove any property that you don't need (e.g if you have default value for it, or you don't have any "Other" features) from the example

- **HosterName**: (Nick)name of server and/or cluster owner
- **ServerName**: Name of the server to be displayed in the launcher. If not specified at all, the name will be taken from Steam query response (which is the same one that you specified as session name in server command-line arguments or GameUserSettings.ini). If its value is *""*, the name will be just the map name (you would want to use that in clusters). This property has no effect if your server is not part of a cluster. You should not set it or set it to anything other than *""* if you use the same json file for all servers in cluster, otherwise all servers in your cluster will be displayed with exactly the same name
- **ClusterName**: Name of the cluster to be displayed if your server is member of one. All servers within the cluster should have the same ClusterName, otherwise it's undefined which one will be used. This property has no effect if your server is not part of a cluster
- **IconUrl**: URL of the cluster's icon, it should be an image with dimensions 128x128 pixels, other sizes are allowed but are not that optimal and will be stretched. The format is irrelevant but JPEG and PNG are preferred
- **Discord**: Invite link for your server/cluster's Discord server. Only URLs starting with *https://discord.gg/* are allowed, otherwise launcher won't display it
- **ServerDescription**: Description object for specific server the file is used for, typically you want it to be unspecified in clusters, but if specified it should describe differences of this server from the rest of servers in the cluster
- **ClusterDescription**: Description object that describes settings/features that apply to all or marjority of servers in the cluster. This property has no effect if your server is not member of any cluster
- Description object properties:
  + **MaxDinoLvl**: The maximum level of wild dinos
  + **Taming**: Taming speed multiplier
  + **Experience**: Experience multiplier
  + **Harvesting**: Harvest amount multiplier
  + **Breeding**: Egg hatch and/or baby mature speed multiplier
  + **Stacks**: The most common multiplier for item stack sizes
  + **Other**: An array of up to 6 extra description lines that you may define yourself

## How does it work?

TEK Wrapper binaries forward all function calls that are not wrapped by it to the original Steam API. On Windows it's achieved by making the same DLL export table as in the original Steam API DLL except that its elements point to functions with the same name in steam_api64_o.dll (which implicitly links it) with exception for wrapped functions that are implemented in TEK Wrapper's dll and call their real counterparts via entries in import table. On Linux there is no separation between exported and imported symbols, so TEKWrapper's binary just doesn't define any symbols it doesn't wrap and loads libsteam_api_o.so via dlopen, so server's lookup for other symbols will end up there; addresses for real counterparts of wrapper functions are obtained via dlsym. SteamAPI_RegisterCallback's wrapper prevents registering GSClientDeny callback and intercepts pointer of GSClientApprove callback for further use. SteamGameServer_Init's wrapper loads mods list and info file URL if present and constructs description line to be sent in Steam server queries, then initializes Steam API and replaces DLL's internal pointer to ISteamGameServer with a wrapper that overrides certain method calls with calls to its own functions and redirects the others to original interface

## License

TEK Wrapper is licensed under the [MIT](LICENSE.TXT) license.
