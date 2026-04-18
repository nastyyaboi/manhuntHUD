#include "plugin.h"
#include "CSprite2d.h"
#include "CTxdStore.h"
#include "CPlayerPed.h"
#include "CFont.h"
#include "CClock.h"
#include "CTimer.h"
#include "CCutsceneMgr.h"
#include "CCamera.h"
#include "extensions/ScriptCommands.h"
#include <string>

using namespace plugin;

extern float breathVisibilityTimerShared;
extern bool bHudEnabledInIni;

class ManStatus {
public:
    static CSprite2d statusSprites[5];
    static bool bLoaded;
    static int nCurrentState;
    static int nPreviousState;
    static int nPendingState;
    static float fStabilityTimer;
    static float fStateLockTimer;
    static float fFadeProgress;

    static float Res(float value) { return value * ((float)RsGlobal.maximumHeight / 960.0f); }

    static std::string GetModFolder() {
        char buffer[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&bLoaded, &hm);
        GetModuleFileNameA(hm, buffer, MAX_PATH);
        std::string path(buffer);
        size_t lastSlash = path.find_last_of("\\/");
        return path.substr(0, lastSlash) + "\\ManhuntHud.SA\\";
    }

    ManStatus() {
        Events::initRwEvent += [] {
            std::string modPath = GetModFolder();
            int txdSlot = CTxdStore::AddTxdSlot("mhud_status");
            if (CTxdStore::LoadTxd(txdSlot, (modPath + "MHud.TXD").c_str())) {
                CTxdStore::AddRef(txdSlot);
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(txdSlot);
                statusSprites[0].SetTexture((char*)"manstand");
                statusSprites[1].SetTexture((char*)"manjog");
                statusSprites[2].SetTexture((char*)"manrun");
                statusSprites[3].SetTexture((char*)"manjump");
                statusSprites[4].SetTexture((char*)"mancrouch");
                CTxdStore::PopCurrentTxd();
                bLoaded = true;
            }
            };

        Events::drawHudEvent += [] { Draw(); };

        Events::shutdownRwEvent += [] {
            for (int i = 0; i < 5; i++) statusSprites[i].Delete();
            bLoaded = false;
            };
    }

    static void DrawSprite(int state, float masterAlpha, CRGBA color) {
        CPlayerPed* player = FindPlayerPed();
        CSprite2d* currentSprite = &statusSprites[state];
        if (!currentSprite || !currentSprite->m_pTexture) return;

        float hX = (float)RsGlobal.maximumWidth - Res(80.0f);
        float hY = (float)RsGlobal.maximumHeight - Res(55.0f);
        float xOffset = Res(75.0f);

        if (state == 0) xOffset -= Res(30.0f);
        else if (state == 1) xOffset -= Res(20.0f);
        else if (state == 2) xOffset -= Res(9.0f);
        else if (state == 4) xOffset -= Res(22.0f);

        if (player->m_fArmour > 1.0f) xOffset += Res(23.0f);
        if (breathVisibilityTimerShared > 0.0f) xOffset += Res(23.0f);

        float w = Res(110.0f);
        float h = Res(220.0f);

        if (state == 3) { w = Res(220.0f); h = Res(220.0f); }
        else if (state == 2) { w = Res(115.0f); h = Res(232.0f); }

        float posX = (hX - xOffset) - (w / 2.0f);
        float posY = (hY - Res(25.0f)) - h - Res(5.0f);

        color.a = (unsigned char)masterAlpha;
        currentSprite->Draw(CRect(posX, posY, posX + w, posY + h), color);
    }

    static void Draw() {
        if (CCutsceneMgr::ms_cutsceneProcessing || TheCamera.m_bWideScreenOn || !bHudEnabledInIni) return;

        CPlayerPed* player = FindPlayerPed();
        if (!player || !bLoaded) return;

        UpdateState(player);

        CRGBA drawColor(255, 255, 255, 255);
        if (player->IsHidden()) {
            drawColor = CRGBA(50, 60, 127, 255);
        }

        int wantedLevel = 0;
        if (player->m_pPlayerData && player->m_pPlayerData->m_pWanted) {
            wantedLevel = player->m_pPlayerData->m_pWanted->m_nWantedLevel;
        }

        if (wantedLevel > 0) {
            int timer = CTimer::m_snTimeInMilliseconds % 300;
            if (timer < 100) drawColor = CRGBA(255, 255, 255, 255);
            else if (timer < 200) drawColor = CRGBA(255, 150, 0, 255);
            else drawColor = CRGBA(200, 0, 0, 255);
        }

        if (fFadeProgress < 1.0f) {
            fFadeProgress += CTimer::ms_fTimeStep * 0.12f;
            if (fFadeProgress > 1.0f) fFadeProgress = 1.0f;

            DrawSprite(nPreviousState, (1.0f - fFadeProgress) * 255.0f, drawColor);
            DrawSprite(nCurrentState, fFadeProgress * 255.0f, drawColor);
        }
        else {
            DrawSprite(nCurrentState, 255.0f, drawColor);
        }
    }

    static void UpdateState(CPlayerPed* p) {
        static unsigned int nOldTime = 0;
        unsigned int nCurrentTime = CTimer::m_snTimeInMilliseconds;
        float fDelta = (float)(nCurrentTime - nOldTime);
        nOldTime = nCurrentTime;
        if (fDelta <= 0.0f || fDelta > 100.0f) fDelta = 16.66f;

        if (fStateLockTimer > 0.0f) {
            fStateLockTimer -= fDelta;
            return;
        }

        float speed = p->m_vecMoveSpeed.Magnitude();
        float vSpeed = p->m_vecMoveSpeed.z;
        int targetState = nCurrentState;

        const float runThreshold = 0.16f;
        const float jogStop = 0.015f;

        if (Command<0x0597>(p)) targetState = 4;
        else if (vSpeed > 0.02f || vSpeed < -0.18f) targetState = 3;
        else {
            if (speed < jogStop) targetState = 0;
            else if (speed > runThreshold) targetState = 2;
            else targetState = 1;
        }

        if (targetState != nCurrentState) {
            if (targetState == nPendingState) {
                fStabilityTimer += fDelta;
                float fRequiredTime = 360.0f;
                if (targetState == 3 || targetState == 0) fRequiredTime = 0.0f;

                if (fStabilityTimer >= fRequiredTime) {
                    nPreviousState = nCurrentState;
                    nCurrentState = targetState;
                    fFadeProgress = 0.0f;
                    fStabilityTimer = 0.0f;
                    fStateLockTimer = 40.0f;
                }
            }
            else {
                nPendingState = targetState;
                fStabilityTimer = 0.0f;
            }
        }
        else fStabilityTimer = 0.0f;
    }
};

CSprite2d ManStatus::statusSprites[5];
bool ManStatus::bLoaded = false;
int ManStatus::nCurrentState = 0;
int ManStatus::nPreviousState = 0;
int ManStatus::nPendingState = 0;
float ManStatus::fStabilityTimer = 0.0f;
float ManStatus::fStateLockTimer = 0.0f;
float ManStatus::fFadeProgress = 1.0f;
ManStatus manStatus;