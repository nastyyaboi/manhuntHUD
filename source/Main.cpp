#define _CRT_SECURE_NO_WARNINGS
#include "plugin.h"
#include "common.h"
#include "CSprite2d.h"
#include "CTxdStore.h"
#include "CStats.h"
#include "CWorld.h"
#include "CFont.h"
#include "CCamera.h"
#include "CText.h"
#include "CHud.h"
#include "CTimer.h"
#include <windows.h>
#include <string>
#include <algorithm>

using namespace plugin;

float breathVisibilityTimerShared = 0.0f;
float breathGracePeriod = 0.0f;
int nToggleKey = 0x4C;
int nPeekKey = 0xBE;
bool bHudEnabledInIni = true;

static const char* weaponTextureNames[] = {
    "fist", "brassknuckle", "golfclub", "nitestick", "knifecur", "batcur", "shovel", "poolcue", "katana", "chnsaw",
    "gun_dildo1", "gun_dildo2", "gun_vibe1", "gun_vibe2", "flower", "cane",
    "grenade", "teargas", "molotov", "null", "null", "null",
    "pistol", "silenced", "desert_eagle", "chromegun", "sawnoff", "shotgspa", "micro_uzi", "mp5lng",
    "ak47", "m4", "tec9", "cuntgun", "sniper", "rocketla", "hsrocket", "flamethrower", "minigun", "satchel",
    "detonator", "spraycan", "fire_ex", "camera", "nvgoog", "irgoog", "parachute"
};

class MHud {
public:
    static CSprite2d outlineSprite;
    static CSprite2d headerSprite;
    static CSprite2d weaponSprites[47];
    static bool bLoaded;
    static float lastBreath;

    static float Res(float value) {
        return value * ((float)RsGlobal.maximumHeight / 960.0f);
    }

    static std::string GetModFolder() {
        char buffer[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&bLoaded, &hm);
        GetModuleFileNameA(hm, buffer, MAX_PATH);
        std::string path(buffer);
        size_t lastSlash = path.find_last_of("\\/");
        return path.substr(0, lastSlash) + "\\ManhuntHud.SA\\";
    }

    void LoadConfig() {
        std::string iniPath = GetModFolder() + "ManhuntHud.SA.ini";
        nToggleKey = GetPrivateProfileIntA("Settings", "ToggleKey", 0x4C, iniPath.c_str());
        nPeekKey = GetPrivateProfileIntA("Settings", "PeekKey", 0xBE, iniPath.c_str());
        bHudEnabledInIni = GetPrivateProfileIntA("Settings", "HudEnabled", 1, iniPath.c_str()) != 0;
    }

    MHud() {
        LoadConfig();
        patch::Nop(0x58F441, 5);

        Events::initRwEvent += [] {
            std::string modPath = GetModFolder();
            int txdSlot = CTxdStore::AddTxdSlot("mhud");
            if (CTxdStore::LoadTxd(txdSlot, (modPath + "MHud.TXD").c_str())) {
                CTxdStore::AddRef(txdSlot);
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(txdSlot);
                outlineSprite.SetTexture((char*)"outline_frame");
                headerSprite.SetTexture((char*)"invstrap");
                CTxdStore::PopCurrentTxd();
            }

            int wepSlot = CTxdStore::AddTxdSlot("mhud_wep");
            if (CTxdStore::LoadTxd(wepSlot, (modPath + "MHudweapons.txd").c_str())) {
                CTxdStore::AddRef(wepSlot);
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(wepSlot);
                for (int i = 0; i < 47; i++) {
                    if (strcmp(weaponTextureNames[i], "null") != 0)
                        weaponSprites[i].SetTexture((char*)weaponTextureNames[i]);
                }
                CTxdStore::PopCurrentTxd();
            }
            bLoaded = true;
            };

        Events::initGameEvent += [] {
            lastBreath = -1.0f;
            breathVisibilityTimerShared = 0.0f;
            breathGracePeriod = 200.0f;
            };

        Events::gameProcessEvent += [] {
            static bool bKeyHeld = false;
            bool bTogglePressed = (GetKeyState(nToggleKey) & 0x8000) != 0;
            if (bTogglePressed && !bKeyHeld) {
                bHudEnabledInIni = !bHudEnabledInIni;
                bKeyHeld = true;
            }
            else if (!bTogglePressed) {
                bKeyHeld = false;
            }
            *(bool*)0xBA6769 = false;
            };

        Events::drawHudEvent += [] {
            if (bLoaded && bHudEnabledInIni) DrawBars();
            };
    }

    static void DrawBars() {
        if (TheCamera.m_bWideScreenOn || *(bool*)0xB5F138) return;
        CPlayerPed* player = FindPlayerPed();
        if (!player || !player->m_pPlayerData || !outlineSprite.m_pTexture) return;

        float screenW = (float)RsGlobal.maximumWidth;
        float screenH = (float)RsGlobal.maximumHeight;
        float s = screenH / 960.0f;
        float barW = Res(11.0f);
        float barMaxH = Res(248.3f);
        float hX = screenW - Res(80.0f);
        float hY = screenH - Res(55.0f);

        auto DrawRoundedBar = [&](float x, float y, float w, float h, CRGBA color) {
            CSprite2d::DrawRect(CRect(x + 1.0f, y - h, x + w - 1.0f, y), color);
            CSprite2d::DrawRect(CRect(x, y - h + 1.0f, x + w, y - 1.0f), color);
            };

        float headW = Res(600.0f);
        float headH = (headW * 512.0f) / 3547.0f;
        float headYPos = hY - barMaxH - Res(70.0f);
        float headLeftEdge = (screenW + Res(320.0f)) - headW;

        int activeSlot = player->m_nSelectedWepSlot;
        CWeapon& activeWep = player->m_aWeapons[activeSlot];
        int weaponId = static_cast<int>(activeWep.m_eWeaponType);

        CRGBA headerColor(255, 255, 255, 200);
        if (weaponId >= 0 && weaponId <= 9) headerColor = CRGBA(0, 255, 0, 200);
        else if (weaponId >= 10 && weaponId <= 15) headerColor = CRGBA(255, 20, 147, 200);
        else if ((weaponId >= 16 && weaponId <= 18) || weaponId == 39) headerColor = CRGBA(255, 255, 0, 200);
        else if (weaponId >= 22 && weaponId <= 24) headerColor = CRGBA(0, 100, 255, 200);
        else if (weaponId >= 25 && weaponId <= 27) headerColor = CRGBA(0, 255, 255, 200);
        else if (weaponId == 28 || weaponId == 29 || weaponId == 32) headerColor = CRGBA(255, 165, 0, 200);
        else if (weaponId == 30 || weaponId == 31) headerColor = CRGBA(255, 0, 0, 200);
        else if (weaponId == 33 || weaponId == 34) headerColor = CRGBA(150, 0, 255, 200);
        else if (weaponId >= 35 && weaponId <= 38) headerColor = CRGBA(37, 150, 190, 200);
        else if (weaponId >= 41 && weaponId <= 46) headerColor = CRGBA(139, 69, 19, 220);

        if (headerSprite.m_pTexture)
            headerSprite.Draw(CRect(headLeftEdge, headYPos - headH, screenW + Res(320.0f), headYPos), headerColor);

        if (activeSlot > 1 && activeWep.m_eWeaponType > 1 && activeWep.m_eWeaponType < 44 && activeWep.m_eWeaponType != 21 && weaponId != 40 && (weaponId < 10 || weaponId > 15)) {
            CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(activeWep.m_eWeaponType, player->GetWeaponSkill());

            float centerX = headLeftEdge + (headW / 2.0f) - Res(137.0f);
            float centerY = headYPos + Res(24.0f);

            CFont::SetFontStyle(FONT_SUBTITLES);
            CFont::SetDropShadowPosition(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, 255));
            CFont::SetScale(0.8f * s, 1.5f * s);
            CFont::SetWrapx(screenW);

            if (activeWep.m_nAmmoTotal < 20000) {
                if (info && info->m_nAmmoClip > 1) {
                    char magStr[16], totalStr[16];
                    sprintf(magStr, "%d", activeWep.m_nAmmoInClip);
                    sprintf(totalStr, " / %d", activeWep.m_nAmmoTotal - activeWep.m_nAmmoInClip);

                    float magW = CFont::GetStringWidth(magStr, true);
                    float totalW = CFont::GetStringWidth(totalStr, true);
                    float startX = centerX - ((magW + totalW) / 2.0f);

                    CFont::SetOrientation(ALIGN_LEFT);
                    if (activeWep.m_nAmmoInClip == info->m_nAmmoClip)
                        CFont::SetColor(CRGBA(123, 189, 130, 255));
                    else
                        CFont::SetColor(CRGBA(255, 255, 255, 255));

                    CFont::PrintString(startX, centerY, magStr);

                    CFont::SetColor(CRGBA(255, 255, 255, 255));
                    CFont::PrintString(startX + magW, centerY, totalStr);
                }
                else {
                    char ammoStr[32];
                    sprintf(ammoStr, "%d", activeWep.m_nAmmoTotal);
                    CFont::SetColor(CRGBA(255, 255, 255, 255));
                    CFont::SetOrientation(ALIGN_CENTER);
                    CFont::PrintString(centerX, centerY, ammoStr);
                }
            }
        }

        if (weaponId >= 0 && weaponId < 47 && weaponSprites[weaponId].m_pTexture) {
            float wX = (hX + (barW / 2.0f) - Res(60.0f)) - Res(45.0f);
            float wY = (headYPos - headH - Res(15.0f)) + Res(118.0f);
            float imgSz = Res(120.0f);
            weaponSprites[weaponId].Draw(wX, wY, wX + imgSz, wY, wX, wY - imgSz, wX + imgSz, wY - imgSz, CRGBA(255, 255, 255, 255));
        }

        float healthStatValue = CStats::GetStatValue(21);
        float maxHealth = std::max(100.0f, 100.0f + (healthStatValue - 569.0f) / 4.31f);
        float healthPerc = std::clamp(player->m_fHealth / maxHealth, 0.0f, 1.0f);
        float armorPerc = std::clamp(player->m_fArmour / 100.0f, 0.0f, 1.0f);

        DrawRoundedBar(hX, hY, barW, barMaxH, CRGBA(37, 35, 19, 150));
        DrawRoundedBar(hX, hY, barW, barMaxH * healthPerc, CRGBA(255, 254, 161, 200));
        outlineSprite.Draw(CRect(hX - Res(3.0f), hY - barMaxH - Res(4.0f), hX + barW + Res(3.0f), hY + Res(4.0f)), CRGBA(255, 255, 255, 255));

        if (player->m_fArmour > 1.0f) {
            float aX = hX - Res(23.0f);
            DrawRoundedBar(aX, hY, barW, barMaxH, CRGBA(26, 32, 15, 150));
            DrawRoundedBar(aX, hY, barW, barMaxH * armorPerc, CRGBA(194, 252, 116, 200));
            outlineSprite.Draw(CRect(aX - Res(3.0f), hY - barMaxH - Res(4.0f), aX + barW + Res(3.0f), hY + Res(4.0f)), CRGBA(255, 255, 255, 255));
        }

        float currentBreath = player->m_pPlayerData->m_fBreath;
        float maxBreath = 1000.0f + (CStats::GetStatValue(22) * 3.0f);
        float breathPerc = std::clamp(currentBreath / maxBreath, 0.0f, 1.0f);

        if (lastBreath < 0.0f) lastBreath = currentBreath;
        if (breathGracePeriod > 0.0f) {
            breathGracePeriod -= CTimer::ms_fTimeStep;
            lastBreath = currentBreath;
        }
        else if (currentBreath < (lastBreath - 0.1f) && breathPerc < 0.98f) {
            breathVisibilityTimerShared = 150.0f;
        }

        if (breathVisibilityTimerShared > 0.0f) {
            if (breathPerc < 0.99f) {
                breathVisibilityTimerShared -= CTimer::ms_fTimeStep;
                float bX = (player->m_fArmour > 1.0f) ? (hX - Res(46.0f)) : (hX - Res(23.0f));
                DrawRoundedBar(bX, hY, barW, barMaxH, CRGBA(0, 10, 30, 150));
                DrawRoundedBar(bX, hY, barW, barMaxH * breathPerc, CRGBA(0, 50, 160, 220));
                outlineSprite.Draw(CRect(bX - Res(3.0f), hY - barMaxH - Res(4.0f), bX + barW + Res(3.0f), hY + Res(4.0f)), CRGBA(255, 255, 255, 255));
            }
        }
        lastBreath = currentBreath;
    }
};

CSprite2d MHud::outlineSprite;
CSprite2d MHud::headerSprite;
CSprite2d MHud::weaponSprites[47];
bool MHud::bLoaded = false;
float MHud::lastBreath = -1.0f;

MHud mHud;