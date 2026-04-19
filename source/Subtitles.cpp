#include "plugin.h"
#include "CHud.h"
#include "CFont.h"
#include "CCamera.h"
#include "Events.h"

using namespace plugin;

#define SCREEN_WIDTH ((float)RsGlobal.maximumWidth)
#define SCREEN_HEIGHT ((float)RsGlobal.maximumHeight)
#define SCREEN_BOTTOM(y) (SCREEN_HEIGHT - (y))

class MHsubs {
public:
    static void DrawSubtitles() {
        if (CHud::m_Message && CHud::m_Message[0] != '\0') {
            CFont::SetProportional(true);
            CFont::SetOrientation(ALIGN_LEFT);
            CFont::SetFontStyle(FONT_SUBTITLES);
            CFont::SetBackground(true, true);
            CFont::SetBackgroundColor(CRGBA(0, 0, 0, 90));

            float resScale = SCREEN_HEIGHT / 1080.0f;
            float scaleMultiplier = 1.6f * resScale;
            CFont::SetScale(0.7f * scaleMultiplier, 1.35f * scaleMultiplier);

            float fPosX = SCREEN_WIDTH * 0.25f;

            float fPosY = SCREEN_HEIGHT * 0.25f;

            float wrapEnd = SCREEN_WIDTH * 0.75f;

            CFont::SetWrapx(wrapEnd);
            CFont::SetDropShadowPosition(1);
            CFont::SetDropColor(CRGBA(0, 0, 0, 255));
            CFont::SetColor(CRGBA(255, 255, 255, 255));

            CFont::PrintString(fPosX, SCREEN_BOTTOM(fPosY), CHud::m_Message);
        }
    }

    MHsubs() {
        patch::SetUChar(0x58C250, 0xC3);
        Events::drawingEvent += [] {
            DrawSubtitles();
            };
    }
} mhSUBS;