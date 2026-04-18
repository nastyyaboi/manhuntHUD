#include "plugin.h"
#include "CFont.h"
#include "CClock.h"
#include "CTimer.h"
#include "CWorld.h"
#include "CCutsceneMgr.h"
#include "CCamera.h"
#include <windows.h>
#include <string>
#include <algorithm>

using namespace plugin;

extern float breathVisibilityTimerShared;
extern int nPeekKey;
extern bool bHudEnabledInIni;

class GrandfatherClock {
public:
    static float nDisplayTimer;
    static float Res(float value) { return value * ((float)RsGlobal.maximumHeight / 960.0f); }

    static void Draw() {
        if (CCutsceneMgr::ms_cutsceneProcessing || TheCamera.m_bWideScreenOn || !bHudEnabledInIni) {
            nDisplayTimer = 0.0f;
            return;
        }

        CPlayerPed* player = FindPlayerPed();
        if (!player || !player->m_pPlayerData) return;

        if (GetKeyState(nPeekKey) & 0x8000) nDisplayTimer = 300.0f;

        if (nDisplayTimer > 0.0f) {
            nDisplayTimer -= CTimer::ms_fTimeStep;
            float s = (float)RsGlobal.maximumHeight / 960.0f;

            int nAlpha = std::clamp((nDisplayTimer < 20.0f) ? (int)(nDisplayTimer * 12.5f) : 255, 0, 255);

            char timeStr[16];
            sprintf(timeStr, "%02d:%02d", CClock::ms_nGameClockHours, CClock::ms_nGameClockMinutes);

            float hX = (float)RsGlobal.maximumWidth - Res(80.0f);
            float hY = (float)RsGlobal.maximumHeight - Res(55.0f);

            float xOffset = Res(60.0f);

            if (player->m_fArmour > 1.0f) xOffset += Res(23.0f);
            if (breathVisibilityTimerShared > 0.0f) xOffset += Res(23.0f);

            CFont::SetOrientation(ALIGN_CENTER);
            CFont::SetFontStyle(FONT_PRICEDOWN);
            CFont::SetScale(0.8f * s, 1.6f * s);
            CFont::SetColor(CRGBA(255, 255, 255, nAlpha));

            CFont::SetDropShadowPosition(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, nAlpha));

            CFont::PrintString(hX - xOffset, hY - Res(25.0f), timeStr);
        }
    }
};

float GrandfatherClock::nDisplayTimer = 0.0f;

class GrandfatherClockPlugin {
public:
    GrandfatherClockPlugin() {
        Events::drawHudEvent += [] { GrandfatherClock::Draw(); };
    }
} grandfatherClockPlugin;