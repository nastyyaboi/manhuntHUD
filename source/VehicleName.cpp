#include "plugin.h"
#include "CFont.h"
#include "CVehicle.h"
#include "CText.h"
#include "CModelInfo.h"
#include "CTimer.h"
#include "CCutsceneMgr.h"
#include "CCamera.h"
#include <windows.h>
#include <algorithm>

using namespace plugin;

extern int nPeekKey; 
extern bool bHudEnabledInIni;

class VehicleName {
public:
    static float nDisplayTimer;
    static CVehicle* pLastVehicle;

    static float Res(float value) {
        return value * ((float)RsGlobal.maximumHeight / 960.0f);
    }

    static void Draw() {
        if (CCutsceneMgr::ms_cutsceneProcessing || TheCamera.m_bWideScreenOn || !bHudEnabledInIni) {
            nDisplayTimer = 0.0f;
            return;
        }

        CPlayerPed* player = FindPlayerPed();
        if (!player) return;

        CVehicle* pVeh = player->m_pVehicle;
        if (!pVeh) {
            nDisplayTimer = 0.0f;
            pLastVehicle = nullptr;
            return;
        }

        unsigned char state = *(unsigned char*)((uintptr_t)player + 0x530);
        bool bPeekPressed = (GetKeyState(nPeekKey) & 0x8000) != 0;

        if (state == 50) { 
            if (pVeh != pLastVehicle || bPeekPressed) {
                nDisplayTimer = 300.0f;
                pLastVehicle = pVeh;
            }
        }
        else {
            pLastVehicle = nullptr;
        }

        if (nDisplayTimer > 0.0f) {
            nDisplayTimer -= CTimer::ms_fTimeStep;

            float s = (float)RsGlobal.maximumHeight / 960.0f;
            int nAlpha = std::clamp((nDisplayTimer < 20.0f) ? (int)(nDisplayTimer * 12.5f) : 255, 0, 255);

            CBaseModelInfo* pBaseInfo = CModelInfo::GetModelInfo(pVeh->m_nModelIndex);
            if (pBaseInfo) {

                char* gxtKey = (char*)((uintptr_t)pBaseInfo + 0x32);
                char* vehName = (char*)TheText.Get(gxtKey);

                if (vehName) {
                    CFont::SetOrientation(ALIGN_RIGHT);
                    CFont::SetFontStyle(FONT_MENU);
                    CFont::SetScale(1.0f * s, 2.0f * s);
                    CFont::SetColor(CRGBA(255, 255, 255, nAlpha));

                    CFont::SetDropShadowPosition(1);
                    CFont::SetDropColor(CRGBA(0, 0, 0, nAlpha));

                    float x = (float)RsGlobal.maximumWidth - Res(30.0f);
                    float y = (float)RsGlobal.maximumHeight / 2.2f;
                    CFont::PrintString(x, y, vehName);
                }
            }
        }
    }
};

float VehicleName::nDisplayTimer = 0.0f;
CVehicle* VehicleName::pLastVehicle = nullptr;

class VehicleNamePlugin {
public:
    VehicleNamePlugin() {
        Events::drawHudEvent += [] {
            VehicleName::Draw();
            };
    }
} vehName;