#include "plugin.h"
#include "common.h"
#include "CMenuManager.h"
#include "CTxdStore.h"
#include "CSprite2d.h"
#include "CPed.h"
#include "CVehicle.h"
#include <string>
#include <windows.h>

using namespace plugin;

static bool isLoaded = false;
static CSprite2d mHudSprite;
static CSprite2d mBarFrame;

const CVector2D mHudPos(155.0f, 150.0f);
const float mHudBaseSize = 93.0f;
const float mHudRingSize = 100.0f;


#define screenHeight (static_cast<float>(RsGlobal.maximumHeight))
#define screenRes(val) (static_cast<float>(val) * (screenHeight / 900.0f))

void RotateVertices(CVector2D* rect, unsigned int numVerts, float x, float y, float angle)
{
    float _cos = cosf(angle);
    float _sin = sinf(angle);
    for (unsigned int i = 0; i < numVerts; i++) {
        float xold = rect[i].x;
        float yold = rect[i].y;
        rect[i].x = x + (xold - x) * _cos + (yold - y) * _sin;
        rect[i].y = y - (xold - x) * _sin + (yold - y) * _cos;
    }
}

class mHudSystem {
public:
    static int txdIndex;

    static std::string fetchPath() {
        char pathBuf[MAX_PATH];
        HMODULE module = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&isLoaded, &module);
        GetModuleFileNameA(module, pathBuf, MAX_PATH);
        std::string fullPath(pathBuf);
        size_t pos = fullPath.find_last_of("\\/");
        return fullPath.substr(0, pos) + "\\ManhuntHud.SA\\";
    }

    static void initiate() {
        if (isLoaded) return;
        isLoaded = true;

        txdIndex = CTxdStore::AddTxdSlot("mhud_internal");
        std::string dir = fetchPath();
        if (CTxdStore::LoadTxd(txdIndex, (dir + "MHud.TXD").c_str())) {
            CTxdStore::AddRef(txdIndex);
            CTxdStore::PushCurrentTxd();
            CTxdStore::SetCurrentTxd(txdIndex);
            mHudSprite.SetTexture((char*)"radardisc");
            mBarFrame.SetTexture((char*)"outline_frame");
            CTxdStore::PopCurrentTxd();
        }

        patch::RedirectCall(0x58AA25, renderHudElement);
        patch::RedirectJump(0x583480, applyHudTranslation);
        patch::RedirectCall(0x58A551, MyDrawRadarPlane);
        patch::RedirectCall(0x58A649, MyDrawPlaneHeight);
        patch::RedirectCall(0x58A77A, MyDrawPlaneHeightBorder);

        patch::Nop(0x58A818, 16);
        patch::Nop(0x58A8C2, 16);
        patch::Nop(0x58A96C, 16);
    }

    static void __fastcall renderHudElement(CSprite2d* self, int, CRect const& rect, CRGBA const& color) {
        if (!isLoaded || !mHudSprite.m_pTexture) return;

        float posX = screenRes(mHudPos.x);
        float posY = screenHeight - screenRes(mHudPos.y);
        float ring = screenRes(mHudRingSize);

        CRect drawArea;
        drawArea.left = posX - ring;
        drawArea.top = posY - ring;
        drawArea.right = posX + ring;
        drawArea.bottom = posY + ring;

        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        mHudSprite.Draw(drawArea, CRGBA(255, 255, 255, 255));
    }

    static void applyHudTranslation(CVector2D* screenOut, CVector2D* worldIn) {
        if (FrontEndMenuManager.m_bDrawRadarOrMap) {
            screenOut->x = FrontEndMenuManager.m_fMapZoom * worldIn->x + FrontEndMenuManager.m_fMapBaseX;
            screenOut->y = FrontEndMenuManager.m_fMapBaseY - FrontEndMenuManager.m_fMapZoom * worldIn->y;
        }
        else {
            screenOut->x = screenRes(mHudPos.x) + (worldIn->x * screenRes(mHudBaseSize));
            screenOut->y = screenHeight - screenRes(mHudPos.y) - (worldIn->y * screenRes(mHudBaseSize));
        }
    }

    static void __fastcall MyDrawRadarPlane(CSprite2d* sprite, int, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, CRGBA const& color)
    {
        if (!sprite || !sprite->m_pTexture) return;

        float rollAngle = 0.0f;
        CVehicle* veh = FindPlayerVehicle(-1, false);

        if (veh && veh->m_matrix) {

            float rightZ = veh->m_matrix->right.z;

            if (rightZ > 1.0f) rightZ = 1.0f;
            if (rightZ < -1.0f) rightZ = -1.0f;


            rollAngle = asinf(rightZ);
        }

        float arrowHalf = screenRes(mHudRingSize * 1.05f);
        float cx = screenRes(mHudPos.x);
        float cy = screenHeight - screenRes(mHudPos.y);

        CVector2D posn[4] = {
            { cx - arrowHalf, cy - arrowHalf },
            { cx + arrowHalf, cy - arrowHalf },
            { cx - arrowHalf, cy + arrowHalf },
            { cx + arrowHalf, cy + arrowHalf }
        };

        RotateVertices(posn, 4, cx, cy, rollAngle);

        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        sprite->Draw(posn[2].x, posn[2].y, posn[3].x, posn[3].y,
            posn[0].x, posn[0].y, posn[1].x, posn[1].y,
            CRGBA(255, 255, 255, 255));
    }

    static float GetNormalizedPlayerHeight() {
        CPed* playa = FindPlayerPed();
        CVehicle* playaVeh = FindPlayerVehicle(-1, false);
        float pZ = (playaVeh) ? playaVeh->GetPosition().z : (playa ? playa->GetPosition().z : 0.0f);
        float heightNorm = (pZ < 0.0f) ? 0.0f : (pZ / 900.0f);
        if (heightNorm > 1.0f) heightNorm = 1.0f;
        return heightNorm;
    }

    static void MyDrawPlaneHeightBorder(CRect const& rect, CRGBA const& color)
    {
        if (!mBarFrame.m_pTexture) return;

        float x = screenRes(mHudPos.x + mHudRingSize + 15.0f);
        float y = screenHeight - screenRes(mHudPos.y);

        float wHalf = screenRes(17.0f * 0.5f * 0.6f);
        float hHalf = screenRes(265.0f * 0.5f * 0.6f);

        mBarFrame.Draw(CRect(x - wHalf, y - hHalf, x + wHalf, y + hHalf), CRGBA(255, 255, 255, 255));

        float heightNorm = GetNormalizedPlayerHeight();
        float barHMax = screenRes(258.0f * 0.5f * 0.6f);
        float barBottom = y + barHMax;
        float barTop = y - barHMax;
        float currentY = barBottom - ((barBottom - barTop) * heightNorm);
        float barW = screenRes(11.0f * 0.5f * 0.6f);

        CSprite2d::DrawRect(CRect(x - barW - screenRes(1.5f), currentY - screenRes(0.5f), x + barW + screenRes(1.5f), currentY + screenRes(0.5f)), CRGBA(255, 255, 255, 255));
    }

    static void MyDrawPlaneHeight(CRect const& rect, CRGBA const& color)
    {
        float heightNorm = GetNormalizedPlayerHeight();
        float x = screenRes(mHudPos.x + mHudRingSize + 15.0f);
        float yCenter = screenHeight - screenRes(mHudPos.y);

        float barW = screenRes(11.0f * 0.5f * 0.6f);
        float barHMax = screenRes(258.0f * 0.5f * 0.6f);

        float barBottom = yCenter + barHMax;
        float barTop = yCenter - barHMax;
        float currentY = barBottom - ((barBottom - barTop) * heightNorm);

        CSprite2d::DrawRect(CRect(x - barW, currentY, x + barW, barBottom), CRGBA(140, 211, 239, 255));
    }

    static void release() {
        if (txdIndex != -1) {
            mHudSprite.Delete();
            mBarFrame.Delete();
            CTxdStore::RemoveTxdSlot(txdIndex);
            txdIndex = -1;
        }
    }
};

int mHudSystem::txdIndex = -1;

class mHudLoader {
public:
    mHudLoader() {
        Events::initRwEvent += [] { mHudSystem::initiate(); };
        Events::shutdownRwEvent += [] { mHudSystem::release(); };
    }
} mHudradar;