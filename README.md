# TEK Wrapper
[![Discord](https://img.shields.io/discord/937821572285206659?style=flat-square&label=Discord&logo=discord&logoColor=white&color=7289DA)](https://discord.gg/47SFqqMBFN)

This repository contains the code for TEK Wrapper

## What is it?

TEK Wrapper is a library for ARK: Survival Evolved dedicated servers that replaces Steam API and some of its functions while redirecting other ones to original Steam API  
It does the following changes to interaction between Steam API and the server:
- All incoming user connections are accepted no matter if they pass Steam checks or not
- All users are assumed to have all Steam app licenses
- Steam server rules query response is modified to include additional information about the server, which consists of the mark to identify that TEK Wrapper is used, active mods list and optionally a URL to file with extra server description that can be displayed in [TEK Launcher](https://github.com/Nuclearistt/TEKLauncher)

## How to use it?

1. Download the library (.dll or .so) file for your OS in [releases](https://github.com/Nuclearistt/TEKWrapper/releases)
2. **Windows**: 
   - Go to **{Server root}\Engine\Binaries\ThirdParty\Steamworks\Steamv132\Win64**, there rename **steam_api64.dll** to **steam_api64_o.dll** and move it to **{Server root}\ShooterGame\Binaries\Win64**
   - Put the downloaded **steam_api64.dll** into **{Server root}\Engine\Binaries\ThirdParty\Steamworks\Steamv132\Win64**

   **Linux**:
   - Go to **{Server root}/Engine/Binaries/Linux**, there rename **libsteam_api.so** to **libsteam_api_o.so**
   - Put the downloaded **libsteam_api.so** into **{Server root}/Engine/Binaries/Linux**
3. Make sure to have *-NoBattlEye* in server command-line parameters, client-side [TEK Injector](https://github.com/Nuclearistt/TEKInjector) doesn't support BattlEye, so people using it won't be able to join your server if you have BE enabled
4. If you want to provide extra information about your server/cluster, add *-TWInfoFileUrl=url* to command-line parameters (for example: *-TWInfoFileUrl=https://example.com/Info.json*). The URL must point to a valid json file that can be accessed by any outside user, its format is described below. If you host a cluster, it's recommended to use the same file URL for all servers of the cluster if they have the same description and naming policy, TEK Launcher takes advantage of that by caching the file so it doesn't have to be downloaded multiple times

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
Every property in the object is optional, you may include only those that you want

- **HosterName**: (Nick)name of server and/or cluster owner
- **ServerName**: Name of the server to be displayed in the launcher. If not specified at all, the name will be taken from Steam query response (which is the same one that you specified as session name in server command-line parameters or GameUserSettings.ini). If its value is *""*, the name will be just the map name (you would want to use that in clusters). This property has no effect if your server is not member of any cluster
- **ClusterName**: Name of the cluster to be displayed if your server is member of one. All servers within the cluster should have the same ClusterName, otherwise it's undefined which one will be used. This property has no effect if your server is not member of any cluster
- **IconUrl**: URL of the cluster's icon, it should be an image with dimensions 128x128 pixels, other sizes are allowed but are not that optimal and will be stretched. The format is irrelevant but JPEG and PNG are preferred
- **Discord**: Invite link for your server/cluster's Discord server. Only URLs starting with *https://discord.gg/* are allowed
- **ServerDescription**: Description object for specific server the file is used for, typically you want it to be unspecified in clusters, but if specified it should describe differences of this server from the rest of servers in the cluster
- **ClusterDescription**: Description object that describes settings/features that apply to all or marjority of servers in the cluster. This property has no effect if your server is not member of any cluster
- Description object properties:
  + **MaxDinoLvl**: The maximum level of wild dinos
  + **Taming**: Taming speed multiplier
  + **Experience**: Experience multiplier
  + **Harvesting**: Harvest amount multiplier
  + **Breeding**: Egg hatch and/or baby mature speed multiplier
  + **Stacks**: The most common multiplier for item stack sizes
  + **Other**: Extra description lines that you may define yourself, this is an array that cannot have more than 6 elements

## How does it work?

The lifetime of TEK Wrapper is the following:
- The server process loads TEK Wrapper library based on its path
- TEK Wrapper's *SteamGameServer_Init* is called, which in turn loads the original steam_api64.dll/libsteam_api.so and imports some of its functions
- Active mods list is searched for in GameUserSettings.ini and command line
- Command line is searched for *-TWInfoFileUrl* parameter
- Certain function pointers in Steam API interfaces are replaced so they point to TEK Wrappers's functions, others are made to forward the call to the same function in original library

## License

TEK Wrapper is licensed under the [MIT](LICENSE.TXT) license.
