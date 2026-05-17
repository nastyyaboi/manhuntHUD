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
#include <algorithm>

using namespace plugin;

class MHudTimer {
public:
    static CSprite2d outlineSprite;
    static bool bLoaded;

    static float Res(float value) {
        return value * ((float)RsGlobal.maximumHeight / 1080.0f);
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
        patch::RedirectCall(0x58FBEE, MissionTimers);
        patch::RedirectJump(0x728640, ProgressBar);

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

    static void __cdecl MissionTimers() {
        if ((CHud::m_BigMessage[4][0] && !CHud::bScriptForceDisplayWithCounters) || CGarages::MessageIDString[0])
            return;

        if (CUserDisplay::OnscnTimer.m_bDisplay != 1)
            return;

        float targetRatio = 1920.0f / 1080.0f;
        float currentScale = (float)RsGlobal.maximumHeight / 1080.0f;
        float aspectDiff = ((float)RsGlobal.maximumWidth - (targetRatio * (float)RsGlobal.maximumHeight));
        float posX = 1741.0f - (aspectDiff / currentScale);

        bool bAnyBarsActive = false;
        for (int i = 0; i < 4; ++i) {
            if (CUserDisplay::OnscnTimer.m_aCounters[i].m_bEnabled && CUserDisplay::OnscnTimer.m_aCounters[i].m_nType == 1) {
                bAnyBarsActive = true;
                break;
            }
        }

        float screenW = (float)RsGlobal.maximumWidth;
        float screenH = (float)RsGlobal.maximumHeight;
        float barMaxH = Res(196.0f);

        float dynamicOffsetX = bAnyBarsActive ? 0.0f : Res(60.0f);

        float hX = screenW - Res(136.0f) + dynamicOffsetX;
        float hY = screenH - Res(110.0f);

        float textBaseY = hY - barMaxH - Res(410.0f);
        float barPosX = hX;
        float textToBarGap = Res(110.0f);
        float lineSpacing = Res(25.0f);   
        float verticalCentering = Res(105.0f); 
        float currentTextY;
        if (bAnyBarsActive) {

            currentTextY = textBaseY - verticalCentering;
        }
        else {
            currentTextY = textBaseY;
        }

        for (int i = 0; i < 4; ++i) {
            if (CUserDisplay::OnscnTimer.m_aCounters[i].m_bEnabled) {
                if (CUserDisplay::OnscnTimer.m_aCounters[i].m_nType == 1) {

                    float progress = (float)atoi(CUserDisplay::OnscnTimer.m_aCounters[i].m_szDisplayedText);

                    ProgressBar(barPosX, textBaseY, 100, 20, progress, 0, 0, 0,
                        HudColour.GetRGB(CUserDisplay::OnscnTimer.m_aCounters[i].m_nColourId, 255), CRGBA(0, 0, 0, 0));


                    if (CUserDisplay::OnscnTimer.m_aCounters[i].m_szDescriptionTextKey[0]) {
                        char* translatedText = const_cast<char*>(TheText.Get(CUserDisplay::OnscnTimer.m_aCounters[i].m_szDescriptionTextKey));
                        if (!translatedText || !translatedText[0]) {
                            translatedText = CUserDisplay::OnscnTimer.m_aCounters[i].m_szDescriptionTextKey;
                        }
                        DrawManhuntTextElement(barPosX - Res(25.0f), textBaseY - Res(25.0f), "", translatedText, CRGBA(255, 255, 255, 255));
                    }


                    barPosX -= Res(45.0f);
                }
                else {
                    DrawManhuntTextElement(hX - textToBarGap, currentTextY,
                        CUserDisplay::OnscnTimer.m_aCounters[i].m_szDisplayedText,
                        CUserDisplay::OnscnTimer.m_aCounters[i].m_szDescriptionTextKey,
                        HudColour.GetRGB(CUserDisplay::OnscnTimer.m_aCounters[i].m_nColourId, 255));

                    currentTextY += lineSpacing;
                }
            }
        }

        if (CUserDisplay::OnscnTimer.m_Clock.m_bEnabled) {
            DrawManhuntTextElement(hX - textToBarGap, currentTextY,
                CUserDisplay::OnscnTimer.m_Clock.m_szDisplayedText,
                CUserDisplay::OnscnTimer.m_Clock.m_szDescriptionTextKey,
                HudColour.GetRGB(HUD_COLOUR_BLUELIGHT, 255));
        }
    }

    static void DrawManhuntTextElement(float x, float y, const char* value, const char* description, CRGBA color) {
        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetScale(Res(0.7f), Res(1.3f));
        CFont::SetEdge(1);
        CFont::SetDropColor(CRGBA(0, 0, 0, 255));
        CFont::SetOrientation(ALIGN_RIGHT);

        float spacing = Res(100.0f);

        if (value && value[0]) {
            CFont::SetColor(color);

            CFont::PrintString(x + spacing, y, const_cast<char*>(value));
        }

        if (description && description[0]) {
            char* textToPrint = const_cast<char*>(TheText.Get(const_cast<char*>(description)));
            if (!textToPrint || !textToPrint[0]) {
                textToPrint = const_cast<char*>(description);
            }
            CFont::SetColor(CRGBA(255, 255, 255, 255));

            CFont::PrintString(x + Res(3.0f), y, textToPrint);
        }
    }

    static void __cdecl ProgressBar(float x, float y, unsigned short width, unsigned char height, float progress,
        signed char progressAdd, unsigned char drawPercentage, unsigned char drawBlackBorder,
        CRGBA color, CRGBA addColor)
    {
        if (x > (float)RsGlobal.maximumWidth * 0.5f && bLoaded && !FrontEndMenuManager.m_bMenuActive) {
            float barW = Res(11.0f);
            float barMaxH = Res(196.0f);
            float safeProgress = std::clamp(progress, 0.0f, 100.0f);
            float progressHeight = (barMaxH * safeProgress) / 100.0f;

            CSprite2d::DrawRect(CRect(x, y - barMaxH, x + barW, y), CRGBA(10, 35, 8, 150));
            if (safeProgress > 0.0f) {
                CSprite2d::DrawRect(CRect(x, y - progressHeight, x + barW, y), CRGBA(33, 146, 21, 200));
            }
            outlineSprite.Draw(CRect(x - Res(3.0f) - 0.5f, y - barMaxH - Res(4.0f) - 0.5f, x + barW + Res(3.0f) + 0.5f, y + Res(4.0f) + 0.5f), CRGBA(255, 255, 255, 255));
        }
        else {
            float fWidth = (float)width;
            float fHeight = (float)height;
            float outlineThickness = 3.0f; 

            CSprite2d::DrawRect(
                CRect(x - outlineThickness, y - outlineThickness, x + fWidth + outlineThickness, y + fHeight + outlineThickness),
                CRGBA(0, 0, 0, 255)
            );
            CSprite2d::DrawRect(CRect(x, y, x + (float)width, y + (float)height), CRGBA(20, 20, 20, 255));

            float clampedProgress = std::clamp(progress, 0.0f, 100.0f);
            float fill = ((float)width * clampedProgress) / 100.0f;

            CSprite2d::DrawRect(CRect(x, y, x + fill, y + (float)height), HudColour.GetRGB(HUD_COLOUR_BLUELIGHT, 255));
        }
    }
};

CSprite2d MHudTimer::outlineSprite;
bool MHudTimer::bLoaded = false;
MHudTimer timer;