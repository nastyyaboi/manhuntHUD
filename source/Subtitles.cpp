#include "plugin.h"
#include "CHud.h"
#include "CFont.h"
#include "CCamera.h"
#include "Events.h"
#include "CTimer.h"
#include "CSprite2d.h"

using namespace plugin;

#define SCREEN_WIDTH ((float)RsGlobal.maximumWidth)
#define SCREEN_HEIGHT ((float)RsGlobal.maximumHeight)
#define SCREEN_BOTTOM(y) (SCREEN_HEIGHT - (y))

struct MissionSceneText {
    static bool bRecapShowing;
    static char capturedTitle[128];
    static float alphaPercent;
    static bool bIsTemporary;
};

class MHsubs {
public:
    static inline unsigned int m_redTextDelayTimer = 0;
    static inline unsigned int m_failBlockExpiry = 0;
    static inline char m_lastMessage[256] = { 0 };

    static void DrawCrtLinesInside(CRect rect, unsigned char boxAlpha) {
        float resScale = SCREEN_HEIGHT / 1080.0f;
        float lineGap = 6.0f * resScale;
        float lineThickness = 1.5f * resScale;
        unsigned char lineAlpha = (unsigned char)(boxAlpha * 0.5f);

        for (float y = rect.top; y < rect.bottom; y += lineGap) {
            float currentLineBottom = (y + lineThickness > rect.bottom) ? rect.bottom : y + lineThickness;
            CSprite2d::DrawRect(CRect(rect.left, y, rect.right, currentLineBottom), CRGBA(0, 0, 0, lineAlpha));
        }
    }

    static void DrawSubtitles() {
        unsigned int currentTime = CTimer::m_snTimeInMilliseconds;

        if (!CHud::m_Message || CHud::m_Message[0] == '\0') {
            m_lastMessage[0] = '\0';
            return;
        }

        bool isRedText = (strncmp(CHud::m_Message, "~r~", 3) == 0 || strncmp(CHud::m_Message, "~R~", 3) == 0);

        if (MissionSceneText::bRecapShowing && strstr(MissionSceneText::capturedTitle, "FAILED")) {
            m_failBlockExpiry = currentTime + 10000;
        }

        if (isRedText) {
            if (MissionSceneText::bRecapShowing) return;
            if (currentTime < m_failBlockExpiry) return;

            if (strcmp(m_lastMessage, CHud::m_Message) != 0) {
                strncpy(m_lastMessage, CHud::m_Message, 255);
                m_redTextDelayTimer = currentTime + 500;
            }
            if (currentTime < m_redTextDelayTimer) return;
        }
        else {
            m_lastMessage[0] = '\0';
        }

        CFont::SetProportional(true);
        CFont::SetOrientation(ALIGN_LEFT);
        CFont::SetFontStyle(FONT_SUBTITLES);

        float resScale = SCREEN_HEIGHT / 1080.0f;
        float scaleMultiplier = 1.6f * resScale;
        float scaleW = 0.7f * scaleMultiplier;
        float scaleH = 1.35f * scaleMultiplier;
        CFont::SetScale(scaleW, scaleH);

        float fPosX = SCREEN_WIDTH * 0.25f;
        float fPosY = SCREEN_HEIGHT * 0.25f;
        float wrapEnd = SCREEN_WIDTH * 0.75f;
        CFont::SetWrapx(wrapEnd);

        int numLines = CFont::GetNumberLines(fPosX, SCREEN_BOTTOM(fPosY), CHud::m_Message);

        float boxHeight = (numLines * (scaleH * 20.0f)) + (12.0f * resScale);
        float rectTop = SCREEN_BOTTOM(fPosY) - (8.0f * resScale);
        CRect subRect(fPosX - (15.0f * resScale), rectTop, wrapEnd + (15.0f * resScale), rectTop + boxHeight);

        CSprite2d::DrawRect(subRect, CRGBA(0, 0, 0, 90));
        DrawCrtLinesInside(subRect, 90);

        CFont::SetBackground(false, false);
        CFont::SetDropShadowPosition(1);
        CFont::SetDropColor(CRGBA(0, 0, 0, 255));
        CFont::SetColor(CRGBA(255, 255, 255, 255));

        CFont::PrintString(fPosX, SCREEN_BOTTOM(fPosY), CHud::m_Message);
    }

    MHsubs() {
        patch::SetUChar(0x58C250, 0xC3);
        Events::drawingEvent += [] { DrawSubtitles(); };
    }
} mhSUBS;