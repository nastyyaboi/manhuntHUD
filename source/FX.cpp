#include "plugin.h"
#include "CSprite2d.h"
#include "CTimer.h"
#include "CTxdStore.h"
#include <string>

using namespace plugin;

#define SCREEN_WIDTH ((float)RsGlobal.maximumWidth)
#define SCREEN_HEIGHT ((float)RsGlobal.maximumHeight)

static std::string GetPluginPath()
{
    char buffer[MAX_PATH];
    HMODULE hm = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetPluginPath, &hm);
    GetModuleFileNameA(hm, buffer, MAX_PATH);
    std::string path(buffer);
    return path.substr(0, path.find_last_of("\\/"));
}

class FXSystem {
public:
    static inline bool Enabled = true;

    static void LoadConfig()
    {
        char path[MAX_PATH];
        sprintf(path, "%s\\ManhuntHud.SA.ini", GetPluginPath().c_str());
        Enabled = GetPrivateProfileIntA("Settings", "FX", 1, path) != 0;
    }
};

class CRTScanlines {
public:
    static void Draw()
    {
        if (!FXSystem::Enabled) return;

        float spacing = 4.5f;
        float thickness = 2.9f;
        float offset = fmod((float)CTimer::m_snTimeInMilliseconds * 0.09f, spacing);

        for (float y = offset; y < SCREEN_HEIGHT; y += spacing)
        {
            CSprite2d::DrawRect(CRect(0.0f, y, SCREEN_WIDTH, y + thickness), CRGBA(0, 0, 0, 12));
        }
    }
};

class FilmGrain {
public:
    static inline int Strength = 25;
    static inline float Scale = 0.8f;
    static inline CSprite2d Sprite;

    static void Draw()
    {
        if (!FXSystem::Enabled || !Sprite.m_pTexture) return;

        float tile = 256.0f * Scale;
        float xo = (float)(rand() % (int)tile) - tile;
        float yo = (float)(rand() % (int)tile) - tile;

        float u1 = 0, u2 = 1, v1 = 0, v2 = 1;
        int r = rand() % 4;
        if (r == 1) std::swap(u1, u2);
        if (r == 2) std::swap(v1, v2);
        if (r == 3) { std::swap(u1, u2); std::swap(v1, v2); }

        for (float x = xo; x < SCREEN_WIDTH; x += tile)
        {
            for (float y = yo; y < SCREEN_HEIGHT; y += tile)
            {
                if (x + tile < 0 || y + tile < 0) continue;
                Sprite.Draw(CRect(x, y, x + tile, y + tile), CRGBA(255, 255, 255, Strength), u1, v1, u2, v1, u1, v2, u2, v2);
            }
        }
    }

    static void LoadTexture()
    {
        char path[MAX_PATH];
        sprintf(path, "%s\\ManhuntHud.SA\\mhud.txd", GetPluginPath().c_str());
        int slot = CTxdStore::AddTxdSlot("fx_grain");
        if (CTxdStore::LoadTxd(slot, path))
        {
            CTxdStore::PushCurrentTxd();
            CTxdStore::SetCurrentTxd(slot);
            Sprite.SetTexture((char*)"noise");
            CTxdStore::PopCurrentTxd();
        }
    }
};

class FXPlugin {
public:
    FXPlugin()
    {
        Events::initGameEvent += [] {
            FXSystem::LoadConfig();
            };

        Events::initRwEvent += [] {
            FilmGrain::LoadTexture();
            };

        Events::drawingEvent += [] {
            CRTScanlines::Draw();
            FilmGrain::Draw();
            };
    }
} fxPlugin;