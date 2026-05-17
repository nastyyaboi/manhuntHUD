#include "plugin.h"
#include "RenderWare.h"
#include "CTxdStore.h"
#include "Patch.h"
#include "CCamera.h"
#include "CHud.h"
#include "CSprite2d.h"
#include "CSprite.h"
#include "CWeaponInfo.h"
#include "CMenuManager.h"
#include "CWeapon.h"
#include <windows.h>
#include <string>

using namespace plugin;

static RwTexture* g_pCustomTex = nullptr;

static std::string GetAsiDir() {
    char buf[MAX_PATH] = {};
    HMODULE hMod = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&GetAsiDir), &hMod);
    GetModuleFileNameA(hMod, buf, MAX_PATH);
    std::string s(buf);
    return s.substr(0, s.find_last_of("\\/") + 1);
}

static void LoadCustomTexture() {
    if (g_pCustomTex) return;
    std::string path = GetAsiDir() + "ManhuntHud.SA\\MHudcrosshair.txd";
    RwStream* stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path.c_str());
    if (!stream) return;
    if (!RwStreamFindChunk(stream, rwID_TEXDICTIONARY, nullptr, nullptr)) {
        RwStreamClose(stream, nullptr);
        return;
    }
}

#define CROSSHAIRS_TOTALSPRITES 4
enum eCrosshairSprites { CROSSHAIR_M16 = 0, CROSSHAIR_ROCKET = 1, CROSSHAIR_SNIPER = 2, CROSSHAIR_VIEWFINDER = 3 };

CSprite2d CrosshairSprites[CROSSHAIRS_TOTALSPRITES];
bool ms_bSpritesLoaded = false;
char* CrosshairsNames[CROSSHAIRS_TOTALSPRITES] = { (char*)"sitem16", (char*)"siterocket", (char*)"scope", (char*)"viewfinder" };

class CHudNew {
public:
    static void Initialise() {
        if (!ms_bSpritesLoaded) {
            int CrosshairSlot = CTxdStore::AddTxdSlot("crosshair");
            CTxdStore::LoadTxd(CrosshairSlot, PLUGIN_PATH((char*)"ManhuntHud.SA\\MHudcrosshair.txd"));
            CTxdStore::AddRef(CrosshairSlot);
            CTxdStore::PushCurrentTxd();
            CTxdStore::SetCurrentTxd(CrosshairSlot);
            for (int i = 0; i < CROSSHAIRS_TOTALSPRITES; i++)
                CrosshairSprites[i].SetTexture(CrosshairsNames[i]);
            CTxdStore::PopCurrentTxd();
            ms_bSpritesLoaded = true;
        }
    }
    static void Shutdown() {
        if (ms_bSpritesLoaded) {
            for (int i = 0; i < CROSSHAIRS_TOTALSPRITES; ++i)
                CrosshairSprites[i].Delete();
            int CrosshairSlot = CTxdStore::FindTxdSlot("crosshair");
            CTxdStore::RemoveTxdSlot(CrosshairSlot);
            ms_bSpritesLoaded = false;
        }
    }
};

void Reticle() {
    CPlayerPed* player = FindPlayerPed(0);
    if (!player || !player->m_pPlayerData || FrontEndMenuManager.m_bMenuActive || !CHud::m_Wants_To_Draw_Hud) return;

    eCamMode Mode = TheCamera.m_aCams[TheCamera.m_nActiveCam].m_nMode;

    if ((int)Mode == 45 || Mode == MODE_AIMWEAPON_ATTACHED) {
        float x = SCREEN_WIDTH * 0.5f;
        float y = SCREEN_HEIGHT * 0.5f;
        float fixedSize = SCREEN_COORD(6.0f);
        CRect rect(x - fixedSize, y - fixedSize, x + fixedSize, y + fixedSize);
        CrosshairSprites[CROSSHAIR_M16].Draw(rect, CRGBA(255, 255, 255, 255));
        return;
    }

    if (Mode == MODE_CAMERA) {
        float size = SCREEN_COORD(512.0f);
        CRect rect(
            (SCREEN_WIDTH / 2.0f) - size,
            (SCREEN_HEIGHT / 2.0f) - size,
            (SCREEN_WIDTH / 2.0f) + size,
            (SCREEN_HEIGHT / 2.0f) + size
        );
        CrosshairSprites[CROSSHAIR_VIEWFINDER].Draw(rect, CRGBA(255, 255, 255, 255));
        return;
    }

    int slot = player->m_nSelectedWepSlot;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(player->m_aWeapons[slot].m_eWeaponType, player->GetWeaponSkill());
    if (!info) return;

    if (Mode == MODE_AIMWEAPON || Mode == MODE_AIMWEAPON_FROMCAR ||
        Mode == MODE_ROCKETLAUNCHER || Mode == MODE_ROCKETLAUNCHER_HS ||
        Mode == MODE_SNIPER) {

        float x = (float)SCREEN_WIDTH * CCamera::m_f3rdPersonCHairMultX;
        float y = (float)SCREEN_HEIGHT * CCamera::m_f3rdPersonCHairMultY;
        float fRadius = player->GetWeaponRadiusOnScreen();
        CRect rect;

        if (info->m_nModelId == 358) {
            float sniperSize = 575.0f;
            rect.left = (SCREEN_WIDTH / 2.0f) - SCREEN_COORD(sniperSize);
            rect.top = (SCREEN_HEIGHT / 2.0f) - SCREEN_COORD(sniperSize);
            rect.right = (SCREEN_WIDTH / 2.0f) + SCREEN_COORD(sniperSize);
            rect.bottom = (SCREEN_HEIGHT / 2.0f) + SCREEN_COORD(sniperSize);
            CrosshairSprites[CROSSHAIR_SNIPER].Draw(rect, CRGBA(255, 255, 255, 255));
        }
        else if (info->m_nModelId == 359 || info->m_nModelId == 360) {
            float rocketSize = 80.0f;
            rect.left = (SCREEN_WIDTH / 2.0f) - SCREEN_COORD(rocketSize);
            rect.top = (SCREEN_HEIGHT / 2.0f) - SCREEN_COORD(rocketSize);
            rect.right = (SCREEN_WIDTH / 2.0f) + SCREEN_COORD(rocketSize);
            rect.bottom = (SCREEN_HEIGHT / 2.0f) + SCREEN_COORD(rocketSize);
            CrosshairSprites[CROSSHAIR_ROCKET].Draw(rect, CRGBA(255, 255, 255, 255));
        }
        else {
            bool bIsAimingAtPed = false;
            if (player->m_pPlayerTargettedPed) bIsAimingAtPed = true;
            else if (player->m_pPlayerData && player->m_pPlayerData->m_bHaveTargetSelected) bIsAimingAtPed = true;
            else {
                CEntity* crosshairEnt = *(CEntity**)0xB79358;
                if (crosshairEnt && (uintptr_t)crosshairEnt > 0x100 && crosshairEnt->m_nType == 1)
                    bIsAimingAtPed = true;
            }

            if (bIsAimingAtPed) {
                float physicalSpeed = sqrtf(player->m_vecMoveSpeed.x * player->m_vecMoveSpeed.x + player->m_vecMoveSpeed.y * player->m_vecMoveSpeed.y);
                float lineLen = SCREEN_COORD(9.0f);
                float lineThick = SCREEN_COORD(1.0f);
                float targetGap = (physicalSpeed * SCREEN_COORD(60.0f)) + (fRadius * SCREEN_COORD(40.0f));
                static float smoothGap = 0.0f;
                smoothGap += (targetGap - smoothGap) * 0.15f;
                CRGBA lockColor(255, 255, 255, 255);
                CSprite2d::DrawRect(CRect(x - (lineThick / 2), y - smoothGap - lineLen, x + (lineThick / 2), y - smoothGap), lockColor);
                CSprite2d::DrawRect(CRect(x - (lineThick / 2), y + smoothGap, x + (lineThick / 2), y + smoothGap + lineLen), lockColor);
                CSprite2d::DrawRect(CRect(x - smoothGap - lineLen, y - (lineThick / 2), x - smoothGap, y + (lineThick / 2)), lockColor);
                CSprite2d::DrawRect(CRect(x + smoothGap, y - (lineThick / 2), x + smoothGap + lineLen, y + (lineThick / 2)), lockColor);
            }
            else {
                float fixedSize = SCREEN_COORD(6.0f);
                rect.left = x - fixedSize; rect.top = y - fixedSize;
                rect.right = x + fixedSize; rect.bottom = y + fixedSize;
                CrosshairSprites[CROSSHAIR_M16].Draw(rect, CRGBA(255, 255, 255, 255));
            }
        }
    }
}

class ReticleEmpty {
public:
    ReticleEmpty() {
        Events::initRwEvent += []() { LoadCustomTexture(); };
        Events::initGameEvent += []() {
            patch::PutRetn(0x58E020); 
            CHudNew::Initialise();
            };
        Events::reInitGameEvent += []() { };
        Events::drawingEvent += []() { Reticle(); };
        Events::shutdownRwEvent += []() { CHudNew::Shutdown(); };
    }
} reticle;