#include "plugin.h"
#include "CSprite2d.h"
#include "CFont.h"
#include "CTxdStore.h"
#include "CMenuManager.h"
#include "CUserDisplay.h"
#include "CHud.h"
#include "CGarages.h"
#include "CText.h"
#include "CHudColours.h"
#include <string>

using namespace plugin;

class MHudTimer {
public:
    static CSprite2d outlineSprite;
    static bool bLoaded;

    static float L(float coord1080) {
        return coord1080 * ((float)RsGlobal.maximumHeight / 1080.0f);
    }

    static float LX(float x1080) {
        float targetRatio = 1920.0f / 1080.0f;
        float diff = ((float)RsGlobal.maximumWidth - (targetRatio * (float)RsGlobal.maximumHeight));
        return (x1080 * ((float)RsGlobal.maximumHeight / 1080.0f)) + diff;
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

    MHudTimer() {
        patch::RedirectCall(0x58FBEE, MyDrawMissionTimers);
        patch::RedirectJump(0x728640, MyDrawProgressBar);

        Events::initRwEvent += [] {
            std::string modPath = GetModFolder();
            int txdSlot = CTxdStore::AddTxdSlot((char*)"mhud_bar");
            if (CTxdStore::LoadTxd(txdSlot, (char*)(modPath + "MHud.TXD").c_str())) {
                CTxdStore::AddRef(txdSlot);
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(txdSlot);
                outlineSprite.SetTexture((char*)"outline_frame");
                CTxdStore::PopCurrentTxd();
                bLoaded = true;
            }
            };

        Events::shutdownRwEvent += [] {
            outlineSprite.Delete();
            bLoaded = false;
            };
    }

    static void __cdecl MyDrawMissionTimers() {
        if ((CHud::m_BigMessage[4][0] && !CHud::bScriptForceDisplayWithCounters) || CGarages::MessageIDString[0])
            return;

        if (CUserDisplay::OnscnTimer.m_bDisplay != 1)
            return;

        float posX = 1741.0f;
        float baseY = 190.0f;
        bool bAnyBarActive = false;

        for (int i = 0; i < 4; ++i) {
            if (CUserDisplay::OnscnTimer.m_aCounters[i].m_bEnabled && CUserDisplay::OnscnTimer.m_aCounters[i].m_nType == 1) {
                bAnyBarActive = true;
                break;
            }
        }

        for (int i = 0; i < 4; ++i) {
            if (CUserDisplay::OnscnTimer.m_aCounters[i].m_bEnabled) {
                if (CUserDisplay::OnscnTimer.m_aCounters[i].m_nType == 1) {
                    float progress = (float)atoi(CUserDisplay::OnscnTimer.m_aCounters[i].m_szDisplayedText);
                    MyDrawProgressBar(posX, baseY, 100, 20, progress, 0, 0, 0, HudColour.GetRGB(CUserDisplay::OnscnTimer.m_aCounters[i].m_nColourId, 255), CRGBA(0, 0, 0, 0));

                    if (CUserDisplay::OnscnTimer.m_aCounters[i].m_szDescriptionTextKey[0]) {
                        char* translatedText = const_cast<char*>(TheText.Get(CUserDisplay::OnscnTimer.m_aCounters[i].m_szDescriptionTextKey));
                        ForcedScmTextPosition(posX, baseY, translatedText);
                    }

                    baseY += 19.0f;
                }
                else {
                    float counterX = posX;
                    if (bAnyBarActive) {
                        counterX = (posX + 19.0f) - 55.0f;
                    }

                    DrawManhuntTextElement(counterX, baseY, CUserDisplay::OnscnTimer.m_aCounters[i].m_szDisplayedText,
                        CUserDisplay::OnscnTimer.m_aCounters[i].m_szDescriptionTextKey,
                        HudColour.GetRGB(CUserDisplay::OnscnTimer.m_aCounters[i].m_nColourId, 255));

                    baseY += 55.0f;
                }
            }
        }

        if (CUserDisplay::OnscnTimer.m_Clock.m_bEnabled) {
            float clockX = posX + 19.0f;
            if (bAnyBarActive) clockX -= 55.0f;

            float clockY = baseY + 120.0f;
            DrawManhuntTextElement(clockX, clockY, CUserDisplay::OnscnTimer.m_Clock.m_szDisplayedText,
                CUserDisplay::OnscnTimer.m_Clock.m_szDescriptionTextKey,
                HudColour.GetRGB(HUD_COLOUR_BLUELIGHT, 255));
        }
    }

    static void DrawManhuntTextElement(float x, float y, char* value, char* descriptionKey, CRGBA color) {
        float drawX = x + 87.8f;
        float shadowOffset = 2.2f;

        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetScale(L(0.60f), L(1.2f));
        CFont::SetAlphaFade(255.0f);
        CFont::SetDropShadowPosition(0);
        CFont::SetEdge(0);
        CFont::SetDropColor(CRGBA(0, 0, 0, 255));

        CFont::SetOrientation(ALIGN_RIGHT);
        CFont::SetColor(CRGBA(0, 0, 0, 255));
        CFont::PrintString(LX(drawX + 22.0f + shadowOffset), L(y + shadowOffset), value);
        CFont::SetColor(color);
        CFont::PrintString(LX(drawX + 22.0f), L(y), value);

        if (descriptionKey && descriptionKey[0]) {
            char* translatedText = const_cast<char*>(TheText.Get(descriptionKey));
            CFont::SetOrientation(ALIGN_LEFT);
            CFont::SetColor(CRGBA(0, 0, 0, 255));
            CFont::PrintString(LX(drawX - 150.0f + shadowOffset), L(y + shadowOffset), translatedText);
            CFont::SetColor(CRGBA(255, 255, 255, 255));
            CFont::PrintString(LX(drawX - 150.0f), L(y), translatedText);
        }
    }

    static void __cdecl ForcedScmTextPosition(float x, float y, char* text) {
        float shadowOffset = 2.2f;
        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetScale(L(0.60f), L(1.2f));
        CFont::SetOrientation(ALIGN_RIGHT);
        CFont::SetDropShadowPosition(0);
        CFont::SetDropColor(CRGBA(0, 0, 0, 255));

        CFont::SetColor(CRGBA(0, 0, 0, 255));
        CFont::PrintString(LX(x + 87.8f - 15.0f + shadowOffset), L(y + 100.0f + shadowOffset), text);
        CFont::SetColor(CRGBA(255, 255, 255, 255));
        CFont::PrintString(LX(x + 87.8f - 15.0f), L(y + 100.0f), text);
    }

    static void __cdecl MyDrawProgressBar(float x, float y, unsigned short width, unsigned char height, float progress,
        signed char progressAdd, unsigned char drawPercentage, unsigned char drawBlackBorder,
        CRGBA color, CRGBA addColor)
    {
        bool bIsMissionBar = (x > 1000.0f && !FrontEndMenuManager.m_bMenuActive);

        if (bIsMissionBar && bLoaded) {
            float fWidth = 18.0f;
            float fHeight = 243.0f;
            float drawX = x + 86.8f;
            float drawY = y;

            float pad = 4.0f;
            float innerH = fHeight - (pad * 2.0f);

            float safeProgress = progress;
            if (safeProgress > 100.0f) safeProgress = 100.0f;
            if (safeProgress < 0.0f) safeProgress = 0.0f;

            float progressHeight = (innerH * safeProgress) / 100.0f;

            CSprite2d::DrawRect(CRect(LX(drawX + pad), L(drawY + pad), LX(drawX + fWidth - pad), L(drawY + fHeight - pad)), CRGBA(10, 35, 8, 200));
            if (safeProgress > 0.0f) {
                CSprite2d::DrawRect(CRect(LX(drawX + pad), L((drawY + fHeight - pad) - progressHeight), LX(drawX + fWidth - pad), L(drawY + fHeight - pad)), CRGBA(33, 146, 21, 255));
            }
            outlineSprite.Draw(CRect(LX(drawX), L(drawY), LX(drawX + fWidth), L(drawY + fHeight)), CRGBA(255, 255, 255, 255));
        }
        else {
            CSprite2d::DrawRect(CRect(x, y, x + (float)width, y + (float)height), CRGBA(0, 0, 0, 150));
            float clampedProgress = progress;
            if (clampedProgress > 100.0f) clampedProgress = 100.0f;
            if (clampedProgress < 0.0f) clampedProgress = 0.0f;
            float fill = ((float)width * clampedProgress) / 100.0f;
            CSprite2d::DrawRect(CRect(x, y, x + fill, y + (float)height), color);
        }
    }
};

CSprite2d MHudTimer::outlineSprite;
bool MHudTimer::bLoaded = false;
MHudTimer manhuntHudModa;