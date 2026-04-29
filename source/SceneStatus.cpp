#include "plugin.h"
#include "CHud.h"
#include "CFont.h"
#include "CTimer.h"
#include "CText.h"
#include "CSprite2d.h"
#include "CCutsceneMgr.h"
#include "CCamera.h"
#include "CStats.h"
#include <windows.h> 

using namespace plugin;

#define SCREEN_WIDTH ((float)RsGlobal.maximumWidth)
#define SCREEN_HEIGHT ((float)RsGlobal.maximumHeight)
#define SCALE_Y(val) (val * (SCREEN_HEIGHT / 1080.0f))

typedef void(__cdecl* AddBigMessage_t)(const char*, unsigned int, unsigned short);
void __cdecl MyAddBigMessage(const char* text, unsigned int time, unsigned short style) {
    if (text && (style == 1 || style == 2)) return;
    ((AddBigMessage_t)0x58C6A5)(text, time, style);
}

class MissionSceneText {
public:
    static inline bool bRecapShowing = false;
    static inline bool bWaitingForInput = false;
    static inline bool bFadingOut = false;
    static inline bool bIsDeathArrest = false;
    static inline bool bIsTemporary = false;
    static inline bool bProcessingEvent = false;
    static inline float alphaPercent = 0.0f;
    static inline unsigned int temporaryTimer = 0;
    static inline char lastProcessedString[128] = "";
    static inline unsigned int stringResetTimer = 0;

    static inline unsigned int missionStartTime = 0;
    static inline unsigned int finalMissionTime = 0;
    static inline bool bTimerActive = false;
    static inline float startKills = 0.0f;
    static inline int finalMissionKills = 0;

    static inline char capturedTitle[128] = "";
    static inline char capturedMissionName[128] = "";
    static inline char capturedFailReason[256] = "";
    static inline char capturedReward[256] = "";

    static inline int  continueKey = VK_SHIFT;
    static inline char continueKeyName[64] = "SHIFT";
    static inline char iniPromptText[128] = "PRESS TO CONTINUE";

    static bool IsMajorEvent(const char* text) {
        if (!text) return false;
        return (strstr(text, "PASSED") || strstr(text, "FAILED") || strstr(text, "WASTED") || strstr(text, "BUSTED"));
    }

    static bool IsAnyResult(const char* text) {
        if (!text || text[0] == '\0') return false;
        return (IsMajorEvent(text) || strstr(text, "Round") || strstr(text, "LEVEL") || strstr(text, "Passed"));
    }

    static bool IsBadReadPtr(void* p) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(p, &mbi, sizeof(mbi))) {
            DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
            return !(mbi.Protect & mask) || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS));
        }
        return true;
    }

    static void LoadConfig() {
        continueKey = GetPrivateProfileIntA("Settings", "ContinueKey", VK_SHIFT, ".\\ManhuntHud.SA.ini");
        GetPrivateProfileStringA("Settings", "ContinueText", "PRESS TO CONTINUE", iniPromptText, sizeof(iniPromptText), ".\\ManhuntHud.SA.ini");
        UINT scanCode = MapVirtualKey(continueKey, MAPVK_VK_TO_VSC);
        LONG lParam = (scanCode << 16);
        if (continueKey >= VK_PRIOR && continueKey <= VK_HELP) lParam |= 0x01000000L;
        GetKeyNameTextA(lParam, continueKeyName, sizeof(continueKeyName));
    }

    static inline unsigned int lastClosedTime = 0;

    static void Update() {
        unsigned int currentTime = CTimer::m_snTimeInMilliseconds;

        if (currentTime > stringResetTimer) lastProcessedString[0] = '\0';

        if (CCutsceneMgr::ms_running) {
            bTimerActive = true;
            missionStartTime = currentTime;
            startKills = CStats::GetStatValue(121);
            return;
        }

        if (!bRecapShowing) {
            if (CHud::m_BigMessage[1][0] != '\0') strncpy(capturedMissionName, CHud::m_BigMessage[1], 127);
            if (CHud::m_Message[0] != '\0' && (strstr(CHud::m_Message, "~r~") || strstr(CHud::m_Message, "~R~"))) {
                strncpy(capturedFailReason, CHud::m_Message, 255);
            }
        }

        CPed* player = FindPlayerPed();
        if (player && (uintptr_t)player > 0x1000 && !IsBadReadPtr(player) && !bProcessingEvent) {
            float fHealth = patch::Get<float>(reinterpret_cast<uintptr_t>(player) + 0x540);
            bool isDead = (fHealth < 1.0f);
            unsigned int pedState = 0;
            if (!isDead) pedState = patch::Get<unsigned int>(reinterpret_cast<uintptr_t>(player) + 0x530);
            bool isBusted = (pedState == 63);

            if (isDead || isBusted) {
                const char* eventName = isDead ? "WASTED" : "BUSTED";
                if (strcmp(lastProcessedString, eventName) != 0) {
                    bProcessingEvent = true;
                    strncpy(capturedTitle, eventName, 127);
                    strncpy(lastProcessedString, eventName, 127);
                    stringResetTimer = currentTime + 5000;
                    bIsDeathArrest = true; bRecapShowing = true;
                    if (bTimerActive) {
                        bTimerActive = false;
                        finalMissionTime = currentTime - missionStartTime;
                        finalMissionKills = (int)(CStats::GetStatValue(121) - startKills);
                    }
                }
            }
        }

        if (!bProcessingEvent && CHud::m_BigMessage[0][0] != '\0') {
            const char* hudMsg = CHud::m_BigMessage[0];

            if (IsAnyResult(hudMsg) && strcmp(lastProcessedString, hudMsg) != 0) {


                bool isMissionResult = (strstr(hudMsg, "PASSED") || strstr(hudMsg, "FAILED") || strstr(hudMsg, "Passed"));
                if (isMissionResult && (currentTime - lastClosedTime < 4000)) {
                    CHud::m_BigMessage[0][0] = '\0'; 
                    return;
                }

                bProcessingEvent = true;
                strncpy(lastProcessedString, hudMsg, 127);
                stringResetTimer = currentTime + 10000;

                char* newlinePos = (char*)strstr(hudMsg, "~n~");
                if (newlinePos) {
                    int titleLen = newlinePos - hudMsg;
                    strncpy(capturedTitle, hudMsg, titleLen > 127 ? 127 : titleLen);
                    capturedTitle[titleLen] = '\0';
                    strncpy(capturedReward, newlinePos + 3, 255);
                }
                else strncpy(capturedTitle, hudMsg, 127);

                bRecapShowing = true;
                if (IsMajorEvent(hudMsg)) {
                    bWaitingForInput = true;
                    if (bTimerActive) {
                        bTimerActive = false;
                        finalMissionTime = currentTime - missionStartTime;
                        finalMissionKills = (int)(CStats::GetStatValue(121) - startKills);
                    }
                }
                else {
                    bIsTemporary = true;
                    temporaryTimer = currentTime + 2500;
                    finalMissionTime = 0;
                }
            }
        }

        if (bRecapShowing) {
            CHud::m_BigMessage[0][0] = '\0';
            CHud::m_BigMessage[1][0] = '\0';

            if (!bFadingOut && alphaPercent < 1.0f) { alphaPercent += 0.05f; if (alphaPercent > 1.0f) alphaPercent = 1.0f; }
            if (!bFadingOut) {
                if (bIsDeathArrest) { if (TheCamera.m_fFadeAlpha > 240.0f) bFadingOut = true; }
                else if (bIsTemporary && currentTime > temporaryTimer) bFadingOut = true;
                else if (bWaitingForInput && (GetAsyncKeyState(continueKey) & 0x8000)) bFadingOut = true;
            }
            if (bFadingOut) {
                alphaPercent -= 0.05f;
                if (alphaPercent <= 0.0f) {
                    alphaPercent = 0.0f;

                    lastClosedTime = currentTime;

                    bRecapShowing = bFadingOut = bIsDeathArrest = bIsTemporary = bProcessingEvent = bWaitingForInput = false;
                    memset(capturedTitle, 0, sizeof(capturedTitle));
                    memset(capturedReward, 0, sizeof(capturedReward));
                    memset(capturedMissionName, 0, sizeof(capturedMissionName));
                    memset(capturedFailReason, 0, sizeof(capturedFailReason));
                }
            }
        }
    }

    static void Draw() {
        if (alphaPercent <= 0.0f || CCutsceneMgr::ms_running) return;

        float resScale = SCREEN_HEIGHT / 1080.0f;
        float centerX = SCREEN_WIDTH / 2.0f;
        float boxHeight = SCALE_Y(420.0f);

        float curTopY = SCALE_Y(110.0f);
        float curBotY = curTopY + boxHeight;

        float boxWidth = boxHeight * 1.7f;
        float boxLeft = centerX - (boxWidth / 2.0f);
        float boxRight = centerX + (boxWidth / 2.0f);

        unsigned char textAlpha = (unsigned char)(255.0f * alphaPercent);
        CRGBA themeColor = (strcmp(capturedTitle, "BUSTED") == 0) ? CRGBA(100, 180, 255, textAlpha) :
            (strstr(capturedTitle, "PASSED") || bIsTemporary) ? CRGBA(255, 220, 0, textAlpha) : CRGBA(255, 60, 60, textAlpha);

        if (!bIsTemporary && !bIsDeathArrest) {
            unsigned char boxAlpha = (unsigned char)(90.0f * alphaPercent);
            CSprite2d::DrawRect(CRect(boxLeft, curTopY, boxRight, curBotY), CRGBA(0, 0, 0, boxAlpha));
            for (float y = curTopY; y < curBotY; y += SCALE_Y(6.0f))
                CSprite2d::DrawRect(CRect(boxLeft, y, boxRight, y + SCALE_Y(2.0f)), CRGBA(0, 0, 0, (unsigned char)(30 * alphaPercent)));
        }

        CFont::SetProportional(true);
        CFont::SetOrientation(ALIGN_CENTER);
        CFont::SetDropColor(CRGBA(0, 0, 0, textAlpha));

        if (bIsDeathArrest) {
            CFont::SetFontStyle(FONT_SUBTITLES);
            float midY = curTopY + (boxHeight / 2.0f) - SCALE_Y(65.0f);
            CFont::SetScale(3.2f * resScale, 6.4f * resScale);
            CFont::SetColor(themeColor);
            CFont::SetEdge(1);
            CFont::SetDropShadowPosition(2);
            CFont::PrintString(centerX, midY, capturedTitle);
        }
        else {
            if (capturedMissionName[0] && !bIsTemporary) {
                CFont::SetEdge(0);
                CFont::SetDropShadowPosition(1);
                CFont::SetFontStyle(FONT_GOTHIC);
                CFont::SetScale(1.1f * resScale, 2.2f * resScale);
                CFont::SetColor(CRGBA(180, 180, 180, textAlpha));
                CFont::PrintString(centerX, curTopY + SCALE_Y(15.0f), capturedMissionName);
            }

            float titleYOffset = bIsTemporary ? SCALE_Y(50.0f) : SCALE_Y(100.0f);
            float titleScaleX = bIsTemporary ? 2.8f : 2.1f;
            float titleScaleY = bIsTemporary ? 4.2f : 3.2f;

            CFont::SetFontStyle(FONT_PRICEDOWN);
            CFont::SetScale(titleScaleX * resScale, titleScaleY * resScale);
            CFont::SetColor(themeColor);
            CFont::SetEdge(1);
            CFont::SetDropShadowPosition(2);
            CFont::PrintString(centerX, curTopY + titleYOffset, capturedTitle);

            CFont::SetEdge(0);
            CFont::SetDropShadowPosition(1);

            float currentY = curTopY + titleYOffset + SCALE_Y(100.0f);

            if (finalMissionTime > 0 && !bIsTemporary) {
                char stats[128]; sprintf(stats, "TIME: %02d:%02d  -  KILLS: %d", (finalMissionTime / 1000) / 60, (finalMissionTime / 1000) % 60, finalMissionKills);
                CFont::SetFontStyle(FONT_MENU);
                CFont::SetScale(0.55f * resScale, 1.10f * resScale);
                CFont::SetColor(CRGBA(255, 255, 255, textAlpha));
                CFont::PrintString(centerX, currentY, stats);
                currentY += SCALE_Y(60.0f);
            }

            if (capturedFailReason[0] != '\0' && strstr(capturedTitle, "FAILED") && !bIsTemporary) {
                CFont::SetFontStyle(FONT_SUBTITLES);
                CFont::SetScale(0.62f * resScale, 1.25f * resScale);
                CFont::SetColor(CRGBA(255, 60, 60, textAlpha));
                CFont::PrintString(centerX, currentY, capturedFailReason);
                currentY += SCALE_Y(45.0f);
            }

            if (capturedReward[0]) {
                CFont::SetFontStyle(FONT_SUBTITLES);
                CFont::SetScale(0.62f * resScale, 1.25f * resScale);
                CFont::SetColor(CRGBA(225, 225, 225, textAlpha));
                CFont::PrintString(centerX, currentY, capturedReward);
            }

            if (bWaitingForInput && !bFadingOut) {
                char prompt[192]; sprintf(prompt, "%s [%s]", iniPromptText, continueKeyName);
                CFont::SetFontStyle(FONT_SUBTITLES);
                CFont::SetScale(0.55f * resScale, 1.10f * resScale);
                CFont::SetColor(CRGBA(160, 160, 160, textAlpha));
                CFont::PrintString(centerX, curBotY - SCALE_Y(50.0f), prompt);
            }
        }
    }

    MissionSceneText() {
        LoadConfig();
        patch::RedirectJump(0x58C6A0, (void*)MyAddBigMessage);
        Events::processScriptsEvent += Update;
        Events::drawHudEvent += Draw;
    }
} gMissionSceneText;