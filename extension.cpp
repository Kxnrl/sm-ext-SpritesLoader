#include "extension.h"
#include "CDetour/detours.h"

SpritesLoader g_Extension;
SMEXT_LINK(&g_Extension);

CDetour* g_BuildSpriteLoadName;

IGameConfig* g_pGameConf = nullptr;

// Just got this wired crash
//https://crash.limetech.org/kzf6jukknaqc

// LTCG??
// https://docs.microsoft.com/en-us/cpp/build/reference/ltcg-link-time-code-generation?view=msvc-160
DETOUR_DECL_STATIC3(DETOUR_BuildSpriteLoadName, int, int, outLen, bool*, bIsAVI, bool*, bIsBIK)
{
    const char* pName; char* pOut;
    __asm {
        mov pName, ecx
        mov pOut, edx
    }

    char file[MAX_PATH];
    char* pFile = &file[0];

    void* addr = DETOUR_STATIC_CALL(DETOUR_BuildSpriteLoadName);
    __asm {
        push bIsBIK
        push bIsAVI
        push MAX_PATH
        mov ecx, pName
        mov edx, pFile
        call addr
    }

    // prevent something shit buggy?
    // like 'sprites/sprites/bluelaser1'
    const size_t len = strlen(pFile);
    if (len > 15 && strncmp(pFile, "sprites", 7) == 0 && strncmp(&file[8], "sprites", 7) == 0)
    {
        // spam logs
        smutils->LogError(myself, "Shit detected -> [%s] from [%s] -> Fixed -> [%s]", pFile, pName, &file[8]);
        strcpy(pOut, &file[8]);
    }
    else
    {
        // back
        strcpy(pOut, pFile);
    }

#ifdef DEBUG
    smutils->LogMessage(myself, "Final -> [%s] and [%s] ", pName, pOut);
#endif
}

bool SpritesLoader::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
    if (!gameconfs->LoadGameConfigFile("SpritesLoader.games", &g_pGameConf, error, maxlength))
    {
        smutils->Format(error, maxlength, "Failed to load gamedata.");
        return false;
    }

    CDetourManager::Init(smutils->GetScriptingEngine(), g_pGameConf);

    g_BuildSpriteLoadName = DETOUR_CREATE_STATIC(DETOUR_BuildSpriteLoadName, "BuildSpriteLoadName")
    if (g_BuildSpriteLoadName == nullptr)
    {
        smutils->Format(error, maxlength, "Failed to create detour for 'BuildSpriteLoadName'");
        SDK_OnUnload();
        return false;
    }
    
    g_BuildSpriteLoadName->EnableDetour();

    // init
    g_pShareSys->RegisterLibrary(myself, "SpritesLoader");

    return true;
}

void SpritesLoader::SDK_OnUnload()
{
    if (g_BuildSpriteLoadName != nullptr)
        g_BuildSpriteLoadName->Destroy();

    gameconfs->CloseGameConfigFile(g_pGameConf);
}