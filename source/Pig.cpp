#include "plugin.h"
#include "CPools.h"
#include "CPed.h"
#include "CVehicle.h"
#include "CRadar.h"
#include "CSprite2d.h"
#include "CTimer.h"
#include "CTxdStore.h"

using namespace plugin;

static CSprite2d spritePed;
static CSprite2d spriteCar;
static CSprite2d spriteHeli;
static bool bPigRadarReady = false;

class PigRadar {
public:
    static std::string GetModFolder() {
        char buffer[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetModFolder, &hm);
        GetModuleFileNameA(hm, buffer, MAX_PATH);
        std::string path(buffer);
        size_t lastSlash = path.find_last_of("\\/");
        return path.substr(0, lastSlash) + "\\ManhuntHud.SA\\";
    }

    PigRadar() {
        Events::initRwEvent += [] {
            std::string txdPath = GetModFolder() + "MHud.TXD";
            int pigTxdSlot = CTxdStore::AddTxdSlot("pig_radar_internal");
            if (CTxdStore::LoadTxd(pigTxdSlot, txdPath.c_str())) {
                CTxdStore::AddRef(pigTxdSlot);
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(pigTxdSlot);
                spritePed.SetTexture((char*)"radar_level");
                spriteCar.SetTexture((char*)"radar_police_chase");
                spriteHeli.SetTexture((char*)"radar_police_heli");
                CTxdStore::PopCurrentTxd();
                bPigRadarReady = true;
            }
            };

        Events::shutdownRwEvent += [] {
            if (bPigRadarReady) {
                spritePed.Delete(); spriteCar.Delete(); spriteHeli.Delete();
                int slot = CTxdStore::FindTxdSlot("pig_radar_internal");
                if (slot != -1) CTxdStore::RemoveTxdSlot(slot);
                bPigRadarReady = false;
            }
            };

        Events::drawRadarEvent += [] {
            if (bPigRadarReady) ExecuteDrawLoop();
            };
    }

    static void ExecuteDrawLoop() {
        CPlayerPed* player = FindPlayerPed();
        if (!player) return;

        int wantedLevel = 0;
        if (player->m_pPlayerData && player->m_pPlayerData->m_pWanted) {
            wantedLevel = player->m_pPlayerData->m_pWanted->m_nWantedLevel;
        }

        CVehicle* playerVeh = player->m_pVehicle;

        for (CPed* ped : CPools::ms_pPedPool) {
            if (ped && ped->m_fHealth > 1.0f) {
                int m = ped->m_nModelIndex;
                if (m >= 280 && m <= 288) {
                    if (ped == player) continue;

                    bool isSeated = false;
                    if (ped->m_pVehicle) {
                        if (ped->m_pVehicle->m_pDriver == ped) isSeated = true;
                        else {
                            for (int i = 0; i < 8; i++) {
                                if (ped->m_pVehicle->m_apPassengers[i] == ped) { isSeated = true; break; }
                            }
                        }
                    }
                    if (!isSeated) DrawRotatedBlip(ped->GetPosition(), 6.0f, wantedLevel, &spritePed, 0.0f);
                }
            }
        }

        if (wantedLevel > 0) {
            float angle = (float)CTimer::m_snTimeInMilliseconds * 0.004f;
            for (CVehicle* veh : CPools::ms_pVehiclePool) {
                if (veh && veh->m_fHealth > 1.0f && veh != playerVeh) {
                    int vId = veh->m_nModelIndex;
                    bool isHeli = (vId == 497 || vId == 425);
                    bool isPolice = (vId == 596 || vId == 597 || vId == 598 || vId == 599 ||
                        vId == 490 || vId == 528 || vId == 427 || vId == 432 ||
                        vId == 433 || vId == 523 || vId == 430 || isHeli);

                    if (isPolice && (isHeli || veh->m_pDriver)) {
                        DrawRotatedBlip(veh->GetPosition(), 13.0f, wantedLevel, (isHeli ? &spriteHeli : &spriteCar), angle);
                    }
                }
            }
        }
    }

    static void DrawRotatedBlip(CVector worldPos, float size, int wantedLevel, CSprite2d* sprite, float angle) {
        CVector2D radarPoint, screenPos;
        CRadar::TransformRealWorldPointToRadarSpace(radarPoint, { worldPos.x, worldPos.y });
        if (sqrtf(radarPoint.x * radarPoint.x + radarPoint.y * radarPoint.y) > 1.0f) return;

        CRadar::TransformRadarPointToScreenSpace(screenPos, radarPoint);
        float scale = (RsGlobal.maximumHeight / 448.0f);
        float halfSide = (size * scale) / 2.0f;

        CRGBA drawColor(255, 255, 255, 255);
        if (wantedLevel > 0) {
            int timer = CTimer::m_snTimeInMilliseconds % 300;
            if (timer < 100)      drawColor = CRGBA(255, 165, 0, 255);
            else if (timer < 200) drawColor = CRGBA(255, 150, 0, 255);   
            else                  drawColor = CRGBA(200, 0, 0, 255);     
        }

        float s = sinf(angle); float c = cosf(angle);
        RwIm2DVertex v[4];
        float ox[4] = { -halfSide, halfSide, halfSide, -halfSide };
        float oy[4] = { -halfSide, -halfSide, halfSide, halfSide };
        float uv_u[4] = { 0.0f, 1.0f, 1.0f, 0.0f }, uv_v[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

        unsigned int finalColor = drawColor.ToInt();

        for (int i = 0; i < 4; i++) {
            v[i].x = screenPos.x + (ox[i] * c - oy[i] * s);
            v[i].y = screenPos.y + (ox[i] * s + oy[i] * c);
            v[i].z = 0.0f; v[i].rhw = 1.0f;
            v[i].u = uv_u[i]; v[i].v = uv_v[i];
            v[i].emissiveColor = finalColor;
        }

        if (sprite && sprite->m_pTexture) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)sprite->m_pTexture->raster);
            RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, v, 4);
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        }
    }
} pigradar;