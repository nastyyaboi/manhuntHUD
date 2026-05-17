#include "plugin.h"
#include "common.h"
#include "CStats.h"
#include "CFont.h"
#include "CPed.h"
#include "CTxdStore.h"
#include "CPad.h"
#include "CClock.h"
#include "CVehicle.h"
#include "CWeapon.h"
#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>

using namespace plugin;

static CSprite2d outlineSprite;
static bool bLoaded = false;

static constexpr float BAR_W = 196.0f;
static constexpr float BAR_H = 10.0f;
static constexpr float SPACING = 52.0f;
static constexpr uint8_t MSG_ALPHA = 110;

static const char* weaponNames[] = {
    "Fist", "Brass Knuckles", "Golf Club", "Nightstick", "Knife", "Baseball Bat", "Shovel", "Pool Cue", "Katana", "Chainsaw",
    "Dildo", "Dildo", "Vibrator", "Vibrator", "Flowers", "Cane",
    "Grenade", "Tear Gas", "Molotov", "", "", "",
    "Pistol", "Silenced Pistol", "Desert Eagle", "Shotgun", "Sawed-off", "Combat Shotgun", "Micro Uzi", "MP5",
    "AK-47", "M4", "Tec-9", "Rifle", "Sniper Rifle", "Rocket Launcher", "HS Rocket Launcher", "Flamethrower", "Minigun", "Satchel",
    "Detonator", "Spraycan", "Fire Extinguisher", "Camera", "Night Vision", "Infrared", "Parachute"
};

static const char* dayNames[] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
};

static inline float Res(float v) {
    return v * (static_cast<float>(RsGlobal.maximumHeight) / 1080.0f);
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

void LoadResources() {
    if (bLoaded) return;
    std::string txdPath = GetModFolder() + "MHud.TXD";

    int slot = CTxdStore::AddTxdSlot((char*)"mhud_bar");
    if (CTxdStore::LoadTxd(slot, (char*)txdPath.c_str())) {
        CTxdStore::AddRef(slot);
        CTxdStore::PushCurrentTxd();
        CTxdStore::SetCurrentTxd(slot);
        outlineSprite.SetTexture((char*)"outline_frame");
        CTxdStore::PopCurrentTxd();
        bLoaded = true;
    }
}

bool IsFirearm(int weaponId) {
    return (weaponId >= 22 && weaponId <= 46);
}

void DrawStatBar(float x, float y, const char* name, float val, CRGBA col) {
    float w = Res(BAR_W);
    float h = Res(BAR_H);

    CFont::SetFontStyle(FONT_SUBTITLES);
    CFont::SetScale(Res(0.72f), Res(1.30f));
    CFont::SetColor(CRGBA(255, 255, 255, 255));
    CFont::SetProportional(true);
    CFont::SetOrientation(ALIGN_LEFT);
    CFont::SetCentreSize(Res(640.0f));
    CFont::SetDropShadowPosition(1);
    CFont::SetDropColor(CRGBA(0, 0, 0, 255));
    CFont::SetBackground(false, false);

    CFont::PrintString(x, y - h - Res(31.0f), (char*)name);

    CSprite2d::DrawRect(CRect(x, y - h, x + w, y), CRGBA(8, 25, 35, 180));
    CSprite2d::DrawRect(CRect(x, y - h, x + w, y), CRGBA(col.r, col.g, col.b, 45));

    if (val > 0.0f) {
        float progressWidth = w * std::clamp(val / 1000.0f, 0.0f, 1.0f);
        CSprite2d::DrawRect(CRect(x, y - h, x + progressWidth, y), col);
    }

    if (outlineSprite.m_pTexture) {
        outlineSprite.Draw(
            CRect(x - Res(3.0f), y - h - Res(4.0f), x + w + Res(3.0f), y + Res(4.0f)),
            CRGBA(255, 255, 255, 255),
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f
        );
    }
}

void DrawstatsTab() {
    CPad* pad = CPad::GetPad(0);
    if (!pad || !pad->GetDisplayVitalStats(0)) return;

    CPed* player = FindPlayerPed();
    if (!player) return;

    static DWORD lastShotTime = 0;
    const DWORD hideDuration = 350;

    bool inVehicle = false;
    if (player->m_pVehicle) {
        if (player->m_pVehicle->m_pDriver == player) {
            inVehicle = true;
        }
        else {
            for (int i = 0; i < 8; i++) {
                if (player->m_pVehicle->m_apPassengers[i] == player) {
                    inVehicle = true;
                    break;
                }
            }
        }
    }

    int activeSlot = player->m_nSelectedWepSlot;
    int weaponId = player->m_aWeapons[activeSlot].m_eWeaponType;

    bool isPassive = (player->m_nMoveState == 0 || player->m_nMoveState == 2); 
    bool isNotFighting = !(pad->NewState.ButtonCircle || pad->GetTarget() || (inVehicle && pad->GetCarGunFired()));

    if (!inVehicle && isPassive && isNotFighting) {
        lastShotTime = 0;
    }

    if (IsFirearm(weaponId)) {
        bool shouldHide = false;

        if (player->m_pPlayerData && (player->m_pPlayerData->m_bFreeAiming || pad->GetTarget())) {
            shouldHide = true;
        }

        if (!shouldHide && activeSlot >= 0 && activeSlot < 13) {
            CWeapon* weapon = &player->m_aWeapons[activeSlot];
            if (weapon->m_nState == 2 || weapon->m_nState == 3 || isNotFighting == false) {
                shouldHide = true;
            }
        }

        if (shouldHide) lastShotTime = GetTickCount();
        if (GetTickCount() - lastShotTime < hideDuration) return;
    }

    LoadResources();

    struct StatItem { const char* name; float value; };
    std::vector<StatItem> stats;

    if (weaponId >= 22 && weaponId <= 34) {
        stats.push_back({ weaponNames[weaponId], CStats::GetStatValue(weaponId + 47) });
    }

    stats.push_back({ "Lung Capacity", CStats::GetStatValue(225) });
    stats.push_back({ "Flying Skill", CStats::GetStatValue(223) });
    stats.push_back({ "Cycling Skill", CStats::GetStatValue(230) });
    stats.push_back({ "Bike Skill", CStats::GetStatValue(229) });
    stats.push_back({ "Driving Skill", CStats::GetStatValue(160) });
    stats.push_back({ "Muscle", CStats::GetStatValue(23) });
    stats.push_back({ "Fat", CStats::GetStatValue(21) });
    stats.push_back({ "Stamina", CStats::GetStatValue(22) });
    stats.push_back({ "Total Respect", CStats::GetStatValue(64) });

    float w = Res(BAR_W);
    float x = Res(233.5f) - (w / 2.0f);
    float anchorY = static_cast<float>(RsGlobal.maximumHeight) - Res(138.5f + 30.0f);

    float totalListHeight = (static_cast<float>(stats.size()) * Res(SPACING));
    float timestampBoxHeight = Res(46.0f);
    float boxTop = anchorY - totalListHeight - Res(5.0f);
    float boxBottom = anchorY + timestampBoxHeight + Res(20.0f);

    CRect mainBox(x - Res(15.0f), boxTop, x + w + Res(15.0f), boxBottom);
    CSprite2d::DrawRect(mainBox, CRGBA(0, 0, 0, MSG_ALPHA));

    float grayBoxY = anchorY + Res(17.0f);
    float grayBoxH = Res(32.0f);
    CRect grayBox(mainBox.left, grayBoxY, mainBox.right, grayBoxY + grayBoxH);
    CSprite2d::DrawRect(grayBox, CRGBA(180, 180, 180, 220));

    float step = Res(5.0f);
    for (float i = mainBox.top; i < mainBox.bottom; i += step) {
        float lineH = (i + Res(1.2f) > mainBox.bottom) ? mainBox.bottom : i + Res(1.2f);
        CSprite2d::DrawRect(CRect(mainBox.left, i, mainBox.right, lineH), CRGBA(0, 0, 0, 60));
    }

    CRGBA statBlue(21, 106, 146, 200);
    float currentY = anchorY;
    for (const auto& stat : stats) {
        DrawStatBar(x, currentY, stat.name, stat.value, statBlue);
        currentY -= Res(SPACING);
    }

    char timeStr[32];
    int dayIdx = std::clamp((int)CClock::CurrentDay - 1, 0, 6);
    sprintf(timeStr, "%s %02d:%02d", dayNames[dayIdx], CClock::ms_nGameClockHours, CClock::ms_nGameClockMinutes);

    CFont::SetFontStyle(FONT_MENU);
    CFont::SetScale(Res(0.55f), Res(1.20f));
    CFont::SetColor(CRGBA(0, 0, 0, 255));
    CFont::SetDropShadowPosition(0);
    CFont::SetProportional(true);
    CFont::SetOrientation(ALIGN_CENTER);

    float centerX = grayBox.left + (grayBox.right - grayBox.left) / 2.0f;
    CFont::PrintString(centerX, grayBox.top + Res(4.0f), timeStr);
}

class statsTab {
public:
    statsTab() {
        patch::PutRetn(0x589650);
        Events::drawHudEvent += DrawstatsTab;

        Events::shutdownRwEvent += []() {
            if (bLoaded) {
                outlineSprite.Delete();
                bLoaded = false;
            }
            };
    }
} playerstatstab;