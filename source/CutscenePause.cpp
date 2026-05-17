#include "plugin.h"
#include "CCutsceneMgr.h"
#include "CSprite2d.h"
#include "CTimer.h"
#include "CFont.h"
#include <string>
#include <windows.h>

using namespace plugin;

class CutscenePause {
public:
    static inline bool bIsPaused = false;
    static inline bool bEnabled = true;
    static inline unsigned int nCSKey = 80; 
    static inline unsigned int lastToggleTime = 0;
    static inline float fFadeAlpha = 0.0f;
    static inline float fVisibilityTimer = 0.0f;
    static inline bool bHintShown = false;

    static std::string GetConfigPath() {
        char buffer[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetConfigPath, &hm);
        GetModuleFileNameA(hm, buffer, MAX_PATH);
        std::string path(buffer);
        return path.substr(0, path.find_last_of("\\/")) + "\\ManhuntHud.SA.ini";
    }

    CutscenePause() {
        Events::initGameEvent += [] {
            LoadConfig();
            };

        Events::drawHudEvent += [] {

            unsigned int cutsceneState = *(unsigned int*)0xB5D4B0;

            if (!bEnabled || !CCutsceneMgr::ms_running || cutsceneState == 3) {
                if (bIsPaused) {
                    bIsPaused = false;
                    CTimer::m_UserPause = false;
                }
                fFadeAlpha = 0.0f;
                fVisibilityTimer = 0.0f;
                bHintShown = false;
                return;
            }

            if (!bHintShown) { fVisibilityTimer = 4.0f; bHintShown = true; }

            if (bIsPaused) {
                fFadeAlpha = 255.0f;
            }
            else {
                if (fVisibilityTimer > 0.0f) {
                    fVisibilityTimer -= CTimer::ms_fTimeStep * 0.01f;
                    if (fFadeAlpha < 255.0f) fFadeAlpha += 12.0f;
                }
                else {
                    if (fFadeAlpha > 0.0f) fFadeAlpha -= 8.0f;
                }
            }

            if (fFadeAlpha > 255.0f) fFadeAlpha = 255.0f;
            if (fFadeAlpha < 0.0f) fFadeAlpha = 0.0f;

            if ((GetAsyncKeyState(nCSKey) & 0x8000) && GetTickCount() > lastToggleTime + 500) {
                bIsPaused = !bIsPaused;
                if (!bIsPaused) { fVisibilityTimer = 0.0f; fFadeAlpha = 0.0f; }
                ApplyPauseState();
            }

            if (bIsPaused) {
                CTimer::ms_fTimeStep = 0.0f;
                DrawRGBNoise();

                if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) || (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RETURN) & 0x8000)) {
                    bIsPaused = false;
                    CTimer::m_UserPause = false;
                    lastToggleTime = GetTickCount();
                    return;
                }
            }

            if (fFadeAlpha > 1.0f) DrawVHSTextIndicator(bIsPaused);
            };
    }

private:
    static void ApplyPauseState() {
        lastToggleTime = GetTickCount();
        CTimer::m_UserPause = bIsPaused;
    }

    static void DrawCrtBox(CRect rect, float alpha) {
        float sh = (float)(*(int*)0xC17048);
        float resScale = sh / 1080.0f;
        unsigned char boxAlpha = (unsigned char)(90 * (alpha / 255.0f));
        CSprite2d::DrawRect(rect, CRGBA(0, 0, 0, boxAlpha));

        float lineGap = 6.0f * resScale;
        float lineThickness = 1.5f * resScale;
        unsigned char lineAlpha = (unsigned char)(boxAlpha * 0.5f);

        for (float y = rect.top; y < rect.bottom; y += lineGap) {
            float currentLineBottom = (y + lineThickness > rect.bottom) ? rect.bottom : y + lineThickness;
            CSprite2d::DrawRect(CRect(rect.left, y, rect.right, currentLineBottom), CRGBA(0, 0, 0, lineAlpha));
        }
    }

    static void DrawRGBNoise() {
        float sw = (float)(*(int*)0xC17044), sh = (float)(*(int*)0xC17048);
        for (int i = 0; i < 60; i++) {
            float rx = (float)(rand() % (int)sw), ry = (float)(rand() % (int)sh);
            float rw = (float)(2 + rand() % 8), rh = (float)(1 + rand() % 2);
            CSprite2d::DrawRect(CRect(rx, ry, rx + rw, ry + rh), CRGBA(rand() % 255, rand() % 255, rand() % 255, 80));
        }
    }

    static void DrawVHSTextIndicator(bool paused) {
        float sw = (float)(*(int*)0xC17044), sh = (float)(*(int*)0xC17048);
        float scale = sh / 400.0f;

        float safeOffsetY = sh * 0.09f;
        float safeOffsetX = (sh * 0.07f) + (30.0f * scale);

        char keyName[16];
        UINT scanCode = MapVirtualKey(nCSKey, MAPVK_VK_TO_VSC);
        if (GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName)) == 0) strcpy(keyName, "P");

        char displayLine[128];
        if (paused) sprintf(displayLine, "PAUSED: Press %s to Resume", keyName);
        else sprintf(displayLine, "Press %s to Pause Cutscene", keyName);

        float fontScaleW = scale * 0.32f;
        float fontScaleH = scale * 0.62f;
        CFont::SetScale(fontScaleW, fontScaleH);
        CFont::SetOrientation(ALIGN_LEFT);
        CFont::SetProportional(true);
        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetBackground(false, false);
        CFont::SetDropShadowPosition(1);
        CFont::SetDropColor(CRGBA(0, 0, 0, (unsigned char)fFadeAlpha));

        float textWidth = CFont::GetStringWidth(displayLine, true, false);

        float boxHeight = scale * 12.0f;
        float paddingH = 8.0f * scale;
        float paddingV = 4.0f * scale;

        DrawCrtBox(CRect(safeOffsetX - paddingH, safeOffsetY - paddingV,
            safeOffsetX + textWidth + paddingH, safeOffsetY + boxHeight + paddingV), fFadeAlpha);

        float verticalCenter = safeOffsetY + (boxHeight * 0.05f);

        CFont::SetColor(CRGBA(255, 255, 255, (unsigned char)fFadeAlpha));
        CFont::PrintString(safeOffsetX - 2.0f, verticalCenter - 2.0f, displayLine);

        CFont::SetDropShadowPosition(0);
    }

    static void LoadConfig() {
        std::string iniPath = GetConfigPath();
        bEnabled = GetPrivateProfileIntA("Settings", "EnableCutscenePause", 1, iniPath.c_str()) != 0;
        nCSKey = GetPrivateProfileIntA("Settings", "CSKey", 80, iniPath.c_str());
    }
} cutscenePause;