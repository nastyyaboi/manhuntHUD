#include "plugin.h"
#include "CFont.h"
#include "RenderWare.h"
#include "CWorld.h"
#include "CMenuManager.h"
#include "CCamera.h"
#include "CTimer.h"
#include <windows.h>
#include <algorithm>

using namespace plugin;

extern bool bHudEnabledInIni;
extern int nPeekKey;
float moneyTimerShared = 0.0f;
bool moneyDiffVisibleShared = false;

class MHudMoney {
public:
    static int m_nPreviousMoney;
    static int m_nDisplayMoney; 
    static int m_nDiffMoney;
    static bool bShowMoneyDifference;
    static int nMoneyDifferenceFadeAlpha;
    static float nDisplayTimer;
    static float nDiffTimer;
    static bool bInitialized;

    static float Res(float value) {
        return value * ((float)RsGlobal.maximumHeight / 960.0f);
    }

    static void Draw() {
        if (!bHudEnabledInIni) return;

        int currentMoney = *(int*)0xB7CE50;

        if (!bInitialized) {
            m_nPreviousMoney = currentMoney;
            m_nDisplayMoney = currentMoney;
            bInitialized = true;
            return;
        }

        if (m_nDisplayMoney != currentMoney) {
            int diff = currentMoney - m_nDisplayMoney;
            int step = abs(diff) / 10 + 1;

            if (abs(diff) <= step) m_nDisplayMoney = currentMoney;
            else if (diff > 0) m_nDisplayMoney += step;
            else m_nDisplayMoney -= step;
        }

        if (m_nPreviousMoney != currentMoney) {
            m_nDiffMoney = currentMoney - m_nPreviousMoney;
            if (abs(m_nDiffMoney) > 0) {
                bShowMoneyDifference = true;
                nMoneyDifferenceFadeAlpha = 255;
                nDisplayTimer = 300.0f;
                nDiffTimer = 100.0f;
            }
            m_nPreviousMoney = currentMoney;
        }

        if (GetKeyState(nPeekKey) & 0x8000) nDisplayTimer = 300.0f;

        moneyTimerShared = nDisplayTimer;

        if (nDisplayTimer > 0.0f) {
            nDisplayTimer -= CTimer::ms_fTimeStep;
            float s = (float)RsGlobal.maximumHeight / 960.0f;
            char str[32];

            sprintf_s(str, "$%u", m_nDisplayMoney);

            CFont::SetProportional(true);
            CFont::SetOrientation(ALIGN_RIGHT);
            CFont::SetFontStyle(FONT_PRICEDOWN);
            CFont::SetEdge(1);
            CFont::SetDropShadowPosition(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, 255));
            CFont::SetColor(CRGBA(255, 255, 255, 255));
            CFont::SetScale(1.0f * s, 1.9f * s);

            CFont::PrintString((float)RsGlobal.maximumWidth - Res(70.0f), Res(60.0f), str);
        }

        if (bShowMoneyDifference && m_nDiffMoney != 0) {
            moneyDiffVisibleShared = true;
            float s = (float)RsGlobal.maximumHeight / 960.0f;

            if (nDiffTimer > 0.0f) nDiffTimer -= CTimer::ms_fTimeStep;
            else nMoneyDifferenceFadeAlpha -= (int)(CTimer::ms_fTimeStep * 30);

            if (nMoneyDifferenceFadeAlpha <= 0) {
                nMoneyDifferenceFadeAlpha = 0;
                bShowMoneyDifference = false;
                moneyDiffVisibleShared = false;
            }
            else {
                char diffStr[32];
                if (m_nDiffMoney > 0) sprintf_s(diffStr, "+$%d", m_nDiffMoney);
                else sprintf_s(diffStr, "-$%d", -m_nDiffMoney);

                CFont::SetFontStyle(FONT_PRICEDOWN);
                CFont::SetEdge(1);
                CFont::SetOrientation(ALIGN_RIGHT);
                CFont::SetDropShadowPosition(1);
                CFont::SetDropColor(CRGBA(0, 0, 0, nMoneyDifferenceFadeAlpha));
                CFont::SetColor(m_nDiffMoney > 0 ? CRGBA(0, 180, 0, nMoneyDifferenceFadeAlpha) : CRGBA(180, 0, 0, nMoneyDifferenceFadeAlpha));
                CFont::SetScale(0.6f * s, 1.3f * s);

                CFont::PrintString((float)RsGlobal.maximumWidth - Res(70.0f), Res(100.0f), diffStr);
            }
        }
        else moneyDiffVisibleShared = false;
    }
};

int MHudMoney::m_nPreviousMoney = 0;
int MHudMoney::m_nDisplayMoney = 0;
int MHudMoney::m_nDiffMoney = 0;
bool MHudMoney::bShowMoneyDifference = false;
int MHudMoney::nMoneyDifferenceFadeAlpha = 0;
float MHudMoney::nDisplayTimer = 0.0f;
float MHudMoney::nDiffTimer = 0.0f;
bool MHudMoney::bInitialized = false;

class MHudPlugin {
public:
    MHudPlugin() {
        Events::drawHudEvent += [] {
            if (FrontEndMenuManager.m_bMenuActive || TheCamera.m_bWideScreenOn) return;
            MHudMoney::Draw();
            };
    }
} mHud;