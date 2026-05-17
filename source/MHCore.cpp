#include "plugin.h"
#include "CHud.h"
#include "CFont.h"
#include "CCutsceneMgr.h"
#include "CMenuManager.h"
#include "CCamera.h"
#include "CTimer.h"
#include "CSprite2d.h"

using namespace plugin;

#define SCREEN_WIDTH ((float)RsGlobal.maximumWidth)
#define SCREEN_HEIGHT ((float)RsGlobal.maximumHeight)
#define SCALE_Y(val) (val * (SCREEN_HEIGHT / 1380.0f))
#define SCALE_X(val) (val * (SCREEN_HEIGHT / 1380.0f))

extern bool g_bFinalHudStatus;
struct MissionSceneText { static bool bRecapShowing; };
extern int nPeekKey;

inline void DrawManhuntBox(CRect rect, unsigned char boxAlpha) {
    CSprite2d::DrawRect(rect, CRGBA(0, 0, 0, boxAlpha));

    float resScale = SCREEN_HEIGHT / 1080.0f;
    float lineGap = 6.0f * resScale;
    float lineThickness = 2.0f * resScale;
    unsigned char lineAlpha = (unsigned char)(boxAlpha * 0.4f);

    for (float y = rect.top; y < rect.bottom; y += lineGap) {
        float currentBottom = (y + lineThickness > rect.bottom) ? rect.bottom : y + lineThickness;
        CSprite2d::DrawRect(CRect(rect.left, y, rect.right, currentBottom), CRGBA(0, 0, 0, lineAlpha));
    }
}

class MHudArea {
public:
    static inline float m_visibilityTimer = 0.0f;
    static inline char  m_savedZoneName[64] = { 0 };
    static inline bool  m_isFadeIn = false;

    MHudArea() {
        plugin::patch::RedirectCall(0x58AE5D, CaptureZoneName);
        plugin::patch::RedirectCall(0x58B156, SilenceVehicleName);

        Events::drawingEvent += [] {
            if (MissionSceneText::bRecapShowing) return;
            if (m_savedZoneName[0] == '\0' || FrontEndMenuManager.m_bMenuActive) return;
            if (TheCamera.m_bWideScreenOn || CHud::bScriptDontDisplayRadar) return;

            if (!g_bFinalHudStatus) {
                m_visibilityTimer = 0.0f;
                return;
            }

            bool bIsPeeking = (GetKeyState(nPeekKey) & 0x8000) != 0;

            if (m_isFadeIn) {
                m_visibilityTimer += CTimer::ms_fTimeStep;
                if (m_visibilityTimer >= 300.0f) { m_visibilityTimer = 300.0f; m_isFadeIn = false; }
            }
            else if (bIsPeeking) {
                m_visibilityTimer = 300.0f;
            }
            else if (m_visibilityTimer > 0.0f) {
                m_visibilityTimer -= CTimer::ms_fTimeStep;
            }

            if (m_visibilityTimer > 0.0f) {
                MHudArea::DrawAreaUI(m_savedZoneName, m_visibilityTimer);
            }
            };
    }

    static void CaptureZoneName(float x, float y, char* str) {
        if (str && str[0] != '\0') {
            if (strcmp(m_savedZoneName, str) != 0) {
                strncpy(m_savedZoneName, str, 63);
                m_visibilityTimer = 1.0f;
                m_isFadeIn = true;
            }
        }
    }

    static void DrawAreaUI(char* str, float timer) {
        if (FrontEndMenuManager.m_bMenuActive ||
            CCutsceneMgr::ms_running ||
            !CHud::m_Wants_To_Draw_Hud)
        {
            return;
        }

        int alpha = 255;
        if (m_isFadeIn) {
            alpha = static_cast<int>((timer / 60.0f) * 255.0f);
        }
        else if (timer < 50.0f) {
            alpha = static_cast<int>((timer / 50.0f) * 255.0f);
        }

        if (alpha > 255) alpha = 255;
        if (alpha <= 0) return;

        float resScale = SCREEN_HEIGHT / 1380.0f;
        float scaleMultiplier = 1.8f * resScale;

        float fPosY = (SCREEN_HEIGHT * 0.08f) + SCALE_Y(6.0f);

        float boxWidth = SCALE_X(550.0f);
        float leftEdge = (SCREEN_WIDTH / 2.0f) - (boxWidth / 2.0f);
        float rightEdge = (SCREEN_WIDTH / 2.0f) + (boxWidth / 2.0f);

        CRect rect(leftEdge, fPosY - (0.05f * resScale), rightEdge, fPosY + (54.0f * resScale));

        DrawManhuntBox(rect, (unsigned char)((alpha * 90) / 255));

        CFont::SetProportional(true);
        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetOrientation(ALIGN_CENTER);
        CFont::SetCentreSize(SCREEN_WIDTH);
        CFont::SetScale(0.72f * scaleMultiplier, 1.4f * scaleMultiplier);
        CFont::SetDropShadowPosition(1);
        CFont::SetDropColor(CRGBA(0, 0, 0, alpha));
        CFont::SetColor(CRGBA(255, 255, 255, alpha));

        CFont::PrintString(SCREEN_WIDTH * 0.5f, fPosY, str);
    }

    static void SilenceVehicleName(float x, float y, char* str) {}

};
MHudArea mhcore;