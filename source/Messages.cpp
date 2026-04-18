#include "plugin.h"
#include "CFont.h"
#include "CMessages.h"
#include "CHud.h"
#include "CTimer.h"
#include "CMenuSystem.h"

using namespace plugin;

static constexpr float MessageOriginX = 35.0f;
static constexpr float MessageOriginY = 35.0f;
static constexpr float MessageScaleW = 0.48f;
static constexpr float MessageScaleH = 0.95f;
static constexpr float MessageWidthMax = 210.0f;
static constexpr uint8_t MessageAlpha = 90;

static float Rescale(float value) {
    float screenHeight = (float)*(int*)0xC17048;
    return value * (screenHeight / 448.0f);
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

        CFont::SetScale(Rescale(MessageScaleW), Rescale(MessageScaleH));
        CFont::SetWrapx(Rescale(MessageOriginX + MessageWidthMax));
        CHud::m_fHelpMessageTime = (float)CFont::GetNumberLines(Rescale(MessageOriginX), Rescale(MessageOriginY), CHud::m_pHelpMessageToPrint) + 1.0f;

        CMessages::StringCopy(CHud::m_pLastHelpMessage, CHud::m_pHelpMessage, 400);
    }

    if (CHud::m_nHelpMessageState == 0) return;

    CHud::m_nHelpMessageTimer += (int)(CTimer::ms_fTimeStep * 20.0f);

    if (!CHud::m_bHelpMessagePermanent && (CHud::m_nHelpMessageTimer > CHud::m_fHelpMessageTime * 1500.0f)) {
        CHud::m_nHelpMessageState = 0;
        return;
    }

    CFont::SetAlphaFade(255.0f);
    CFont::SetProportional(true);
    CFont::SetScale(Rescale(MessageScaleW), Rescale(MessageScaleH));
    CFont::SetOrientation(ALIGN_LEFT);
    CFont::SetFontStyle(FONT_SUBTITLES);
    CFont::SetDropShadowPosition(1);
    CFont::SetDropColor(CRGBA(0, 0, 0, 255));
    CFont::SetColor(CRGBA(255, 255, 255, 255));
    CFont::SetBackground(true, true);
    CFont::SetBackgroundColor(CRGBA(0, 0, 0, MessageAlpha));
    CFont::SetWrapx(Rescale(MessageOriginX + MessageWidthMax));

    CFont::PrintString(Rescale(MessageOriginX), Rescale(MessageOriginY), CHud::m_pHelpMessageToPrint);

    CFont::SetDropShadowPosition(0);
}

class MessagesRelay {
public:
    MessagesRelay() {
        patch::RedirectCall(0x58FCFA, ProcessMessageRendering);
    }
} g_MessagesRelay;