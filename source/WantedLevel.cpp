#include "plugin.h"
#include "RenderWare.h"
#include "CMenuManager.h"
#include "CCamera.h"
#include "CWorld.h"
#include "CPlayerData.h"
#include "CSprite2d.h"
#include "CTxdStore.h"
#include "CTimer.h" 
#include <windows.h>
#include <cmath>    

using namespace plugin;

extern float moneyTimerShared;
extern bool moneyDiffVisibleShared;
extern bool bHudEnabledInIni;
extern int nPeekKey;

static RwTexture* (__cdecl* _RwTextureRead)(const char*, const char*) = (RwTexture * (__cdecl*)(const char*, const char*))0x7F3AC0;

class MHWanted {
public:
    static float nWantedTimer;
    static CSprite2d starSprite;
    static int txdSlot;

    static float Res(float value) { return value * ((float)RsGlobal.maximumHeight / 960.0f); }

    static CRGBA GetFlashColor() {
        int timer = CTimer::m_snTimeInMilliseconds % 300;

        if (timer < 100) {
            return CRGBA(255, 255, 255, 255); 
        }
        else if (timer < 200) {
            return CRGBA(255, 150, 0, 255);   
        }
        else {
            return CRGBA(200, 0, 0, 255);    
        }
    }

    static void Draw() {
        if (!bHudEnabledInIni) return;

        int wantedLevel = 0;
        CPlayerPed* player = FindPlayerPed();
        if (player && player->m_pPlayerData) {
            CWanted* pWanted = player->m_pPlayerData->m_pWanted;
            if (pWanted) wantedLevel = pWanted->m_nWantedLevel;
        }

        if (wantedLevel > 0 || (GetKeyState(nPeekKey) & 0x8000)) nWantedTimer = 300.0f;

        if (nWantedTimer > 0.0f && starSprite.m_pTexture) {
            nWantedTimer -= CTimer::ms_fTimeStep;
            float starSize = Res(30.5f);
            float spacing = Res(1.0f);
            float xBase = (float)RsGlobal.maximumWidth - Res(70.0f) - starSize;
            float y = Res(60.0f);

            if (moneyTimerShared > 0.0f) y = moneyDiffVisibleShared ? Res(130.0f) : Res(96.0f);

            CRGBA activeColor = GetFlashColor();
            for (int i = 0; i < 6; i++) {
                float xPos = xBase - (i * (starSize + spacing));
                CRect rect(xPos, y, xPos + starSize, y + starSize);
                if (i < wantedLevel) starSprite.Draw(rect, activeColor);
                else starSprite.Draw(rect, CRGBA(10, 10, 10, 180));
            }
        }
    }

    static void Init() {
        char path[MAX_PATH];
        sprintf_s(path, "%s\\ManhuntHud.SA\\Mhud.txd", PLUGIN_PATH(""));
        txdSlot = CTxdStore::AddTxdSlot("mhud_stars");
        if (CTxdStore::LoadTxd(txdSlot, path)) {
            CTxdStore::AddRef(txdSlot);
            CTxdStore::PushCurrentTxd();
            CTxdStore::SetCurrentTxd(txdSlot);
            starSprite.m_pTexture = _RwTextureRead("star", NULL);
            CTxdStore::PopCurrentTxd();
        }
    }

    static void Shutdown() {
        starSprite.Delete();
        if (txdSlot != -1) CTxdStore::RemoveTxdSlot(txdSlot);
    }
};

float MHWanted::nWantedTimer = 0.0f;
CSprite2d MHWanted::starSprite;
int MHWanted::txdSlot = -1;

class WantedPlugin {
public:
    WantedPlugin() {
        Events::initGameEvent += [] { MHWanted::Init(); };
        Events::shutdownRwEvent += [] { MHWanted::Shutdown(); };
        Events::drawHudEvent += [] {
            if (FrontEndMenuManager.m_bMenuActive || TheCamera.m_bWideScreenOn) return;
            MHWanted::Draw();
            };
    }
} mhWanted;