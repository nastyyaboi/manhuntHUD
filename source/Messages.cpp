#include "plugin.h"
#include "CFont.h"
#include "CMessages.h"
#include "CHud.h"
#include "CTimer.h"
#include "CMenuSystem.h"
#include "CStats.h"
#include "CSprite2d.h"
#include "CText.h"
#include "CHudColours.h"
#include "CCamera.h"

using namespace plugin;

static constexpr float MessageOriginX = 54.0f;
static constexpr float MessageOriginY = 37.0f;
static constexpr float MessageScaleW = 0.48f;
static constexpr float MessageScaleH = 0.95f;
static constexpr float MessageWidthMax = 210.0f;
static constexpr uint8_t MessageAlpha = 90;

static constexpr float StatLabelScaleW = 0.35f;
static constexpr float StatLabelScaleH = 0.70f;
static constexpr float StatBarHeight = 8.0f;
static constexpr float StatSignGap = 4.0f;
static constexpr float StatBarGap = 10.0f;
static constexpr float StatBarYShift = 2.0f;

static float Sc(float value) {
    float screenHeight = static_cast<float>(*(int*)0xC17048);
    return value * (screenHeight / 448.0f);
}

static float ScreenWidth() {
    return static_cast<float>(*(int*)0xC17044);
}

inline void DrawCrtLinesInside(CRect rect, unsigned char boxAlpha) {
    float screenHeight = static_cast<float>(*(int*)0xC17048);
    float resScale = screenHeight / 1080.0f;
    float lineGap = 6.0f * resScale;
    float lineThickness = 1.5f * resScale;
    unsigned char lineAlpha = (unsigned char)(boxAlpha * 0.5f);

    for (float y = rect.top; y < rect.bottom; y += lineGap) {
        float currentLineBottom = (y + lineThickness > rect.bottom) ? rect.bottom : y + lineThickness;
        CSprite2d::DrawRect(CRect(rect.left, y, rect.right, currentLineBottom), CRGBA(0, 0, 0, lineAlpha));
    }
}

static void DrawProgressBarWithDelta(float x, float y, float width, float height, float progress, float progressAdd, CRGBA fillColor, CRGBA addColor) {
    CSprite2d::DrawBarChart(x, y, width, height, progress, progressAdd, false, true, fillColor, addColor);
}

static void ProcessMessageRendering() {
    if (!CHud::m_pHelpMessage[0]) {
        CHud::m_nHelpMessageState = 0;
        return;
    }

    if (!CMessages::StringCompare(CHud::m_pHelpMessage, CHud::m_pLastHelpMessage, 400)) {
        if (CHud::m_nHelpMessageState == 0) {
            reinterpret_cast<void(__thiscall*)(void*, int, float, float)>(0x506EA0)((void*)0xB6BC90, 32, 0.0f, 1.0f);
        }
        CHud::m_nHelpMessageState = 1;
        CHud::m_nHelpMessageTimer = 0;
        CMessages::StringCopy(CHud::m_pHelpMessageToPrint, CHud::m_pHelpMessage, 400);

        CFont::SetScale(Sc(MessageScaleW), Sc(MessageScaleH));
        CFont::SetWrapx(Sc(MessageOriginX + MessageWidthMax));
        CHud::m_fHelpMessageTime = static_cast<float>(CFont::GetNumberLines(Sc(MessageOriginX), Sc(MessageOriginY), CHud::m_pHelpMessageToPrint)) + 1.0f;
        CMessages::StringCopy(CHud::m_pLastHelpMessage, CHud::m_pHelpMessage, 400);
    }

    if (CHud::m_nHelpMessageState == 0) return;
    CHud::m_nHelpMessageTimer += static_cast<int>(CTimer::ms_fTimeStep * 20.0f);

    if (!CHud::m_bHelpMessagePermanent && CHud::m_nHelpMessageTimer > CHud::m_fHelpMessageTime * 1500.0f) {
        CHud::m_nHelpMessageState = 0;
        return;
    }

    float originX = Sc(MessageOriginX);
    float originY = Sc(MessageOriginY);
    float wrapX = Sc(MessageOriginX + MessageWidthMax);

    CFont::SetAlphaFade(255.0f);
    CFont::SetProportional(true);
    CFont::SetOrientation(ALIGN_LEFT);
    CFont::SetFontStyle(FONT_SUBTITLES);
    CFont::SetBackground(false, false);

    if (CHud::m_nHelpMessageStatId) {
        CFont::SetScale(Sc(StatLabelScaleW), Sc(StatLabelScaleH));
        CFont::SetWrapx(wrapX);

        int numLines = CFont::GetNumberLines(originX, originY, CHud::m_pHelpMessageToPrint);

        float lineH = Sc(StatLabelScaleH * 17.5f);
        float topPad = Sc(4.0f);
        float bottomPad = Sc(5.3f);
        float boxHeight = (numLines * lineH) + topPad + bottomPad;

        CRect statRect(originX - Sc(6.0f), originY - Sc(4.0f), wrapX + Sc(6.0f), originY - Sc(4.0f) + boxHeight);

        CSprite2d::DrawRect(statRect, CRGBA(0, 0, 0, MessageAlpha));
        DrawCrtLinesInside(statRect, MessageAlpha);

        CFont::SetColor(CRGBA(255, 255, 255, 255));
        CFont::SetDropShadowPosition(1);
        CFont::SetDropColor(CRGBA(0, 0, 0, 255));
        CFont::PrintString(originX, originY - Sc(4.0f) + topPad, CHud::m_pHelpMessageToPrint);

        char gxtKey[16];
        int sid = CHud::m_nHelpMessageStatId;
        sprintf(gxtKey, (sid < 10) ? "STAT00%d" : (sid < 100) ? "STAT0%d" : "STAT%d", sid);
        const char* statName = TheText.Get(gxtKey);
        float signW = CFont::GetStringWidth(CHud::m_pHelpMessageToPrint, true, false);
        float nameW = CFont::GetStringWidth(const_cast<char*>(statName), true, false);
        float barW = fmaxf(Sc(MessageWidthMax) - (signW + Sc(StatSignGap) + nameW + Sc(StatBarGap)), Sc(20.0f));
        float nameX = originX + signW + Sc(StatSignGap);
        CFont::PrintString(nameX, originY - Sc(4.0f) + topPad, const_cast<char*>(statName));

        float barX = nameX + nameW + Sc(StatBarGap);
        float barY = originY - Sc(4.0f) + topPad + Sc(StatBarYShift) + (Sc(StatLabelScaleH) * 1.5f);
        float progress = (sid == 336) ? static_cast<float>(CallMethodAndReturn<unsigned int, 0x5F6AA0>((void*)(0xC09928 + FindPlayerPed(-1)->m_pPlayerData->m_nPlayerGroup * 0x2D4))) : CStats::GetStatValue(sid);
        float maxVal = (CHud::m_nHelpMessageMaxStatValue <= 0) ? 1000.0f : static_cast<float>(CHud::m_nHelpMessageMaxStatValue);

        CRGBA barColor = (CHud::m_fHelpMessageStatUpdateValue >= 0.0f) ? CRGBA(0, 180, 0, 255) : CRGBA(180, 0, 0, 255);
        DrawProgressBarWithDelta(barX, barY, barW, Sc(StatBarHeight), (1.0f / maxVal) * progress * 100.0f, (1.0f / maxVal) * CHud::m_fHelpMessageStatUpdateValue * 100.0f, HudColour.GetRGB(HUD_COLOUR_WHITE, 255), barColor);
    }
    else {
        CFont::SetScale(Sc(MessageScaleW), Sc(MessageScaleH));
        CFont::SetWrapx(wrapX);

        int numLines = CFont::GetNumberLines(originX, originY, CHud::m_pHelpMessageToPrint);

        float lineH = Sc(MessageScaleH * 18.0f);
        float totalPadding = Sc(10.0f);
        float boxHeight = (numLines * lineH) + totalPadding;
        float vOffset = totalPadding / 2.0f;

        CRect helpRect(originX - Sc(6.0f), originY - Sc(4.0f), wrapX + Sc(6.0f), originY - Sc(4.0f) + boxHeight);

        CSprite2d::DrawRect(helpRect, CRGBA(0, 0, 0, MessageAlpha));
        DrawCrtLinesInside(helpRect, MessageAlpha);

        CFont::SetColor(CRGBA(255, 255, 255, 255));
        CFont::SetDropShadowPosition(1);
        CFont::SetDropColor(CRGBA(0, 0, 0, 255));
        CFont::PrintString(originX, originY - Sc(4.0f) + vOffset, CHud::m_pHelpMessageToPrint);
    }

    CFont::SetDropShadowPosition(0);
    CFont::SetWrapx(ScreenWidth());
}

class MessagesRelay {
public:
    MessagesRelay() {
        patch::RedirectCall(0x58FCFA, ProcessMessageRendering);
    }
} g_MessagesRelay;