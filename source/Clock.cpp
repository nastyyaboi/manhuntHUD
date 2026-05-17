#include "plugin.h"
#include "CFont.h"
#include "CClock.h"
#include "CTimer.h"
#include "CWorld.h"
#include "CHud.h"
#include "CCutsceneMgr.h"
#include "CCamera.h"
#include <windows.h>
#include <string>
#include <algorithm>

using namespace plugin;

extern float breathVisibilityTimerShared;
extern int nPeekKey;
extern bool g_bFinalHudStatus;

static std::string GetPluginPath() {
    char buffer[MAX_PATH];
    HMODULE hm = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetPluginPath, &hm);
    GetModuleFileNameA(hm, buffer, MAX_PATH);
    std::string path(buffer);
    return path.substr(0, path.find_last_of("\\/"));
}

class GrandfatherClock {
public:
    static bool Enabled;
    static float nDisplayTimer;

    static float Res(float value) {
        return value * ((float)RsGlobal.maximumHeight / 960.0f);
    }

    static void LoadConfig() {
        std::string iniPath = GetPluginPath() + "\\ManhuntHud.SA.ini";

        Enabled = GetPrivateProfileIntA("Settings", "GrandfatherClock", 1, iniPath.c_str()) != 0;
    }

    static void Draw() {
        if (!Enabled) return;

        if (CCutsceneMgr::ms_cutsceneProcessing || TheCamera.m_bWideScreenOn || !g_bFinalHudStatus) {
            nDisplayTimer = 0.0f;
            return;
        }

        CPlayerPed* player = FindPlayerPed();
        if (!player || !player->m_pPlayerData) return;

        if (GetKeyState(nPeekKey) & 0x8000)
            nDisplayTimer = 300.0f;

        if (nDisplayTimer > 0.0f) {
            nDisplayTimer -= CTimer::ms_fTimeStep;

            int nAlpha = std::clamp((nDisplayTimer < 20.0f) ? (int)(nDisplayTimer * 12.5f) : 255, 0, 255);

            char timeStr[16];
            sprintf(timeStr, "%02d:%02d", CClock::ms_nGameClockHours, CClock::ms_nGameClockMinutes);

            float hX = (float)RsGlobal.maximumWidth - Res(83.5f);
            float hY = (float)RsGlobal.maximumHeight - Res(70.0f);

            float xOffset = Res(60.0f);

            CFont::SetOrientation(ALIGN_CENTER);
            CFont::SetFontStyle(FONT_PRICEDOWN);
            CFont::SetScale(0.8f * Res(1.0f), 1.6f * Res(1.0f));
            CFont::SetColor(CRGBA(255, 255, 255, nAlpha));

            CFont::SetDropShadowPosition(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, nAlpha));

            CFont::PrintString(hX - xOffset, hY - Res(25.0f), timeStr);
        }
    }
};

bool GrandfatherClock::Enabled = true;
float GrandfatherClock::nDisplayTimer = 0.0f;

class GrandfatherClockPlugin {
public:
    GrandfatherClockPlugin() {
        GrandfatherClock::LoadConfig();

        Events::drawHudEvent += [] {
            GrandfatherClock::Draw();
            };
    }
} grandfatherClock;