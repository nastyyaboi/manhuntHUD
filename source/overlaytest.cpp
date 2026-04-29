#include "plugin.h"
#include "common.h"
#include "CTxdStore.h"
#include "CSprite2d.h"
#include "Events.h"

using namespace plugin;

class ManhuntHud {
public:
    static inline CSprite2d mhSprite;

    // Helper to locate the folder where TXD is stored
    static std::string GetModFolder() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(GetModuleHandleA("ManhuntHud.SA.asi"), buffer, MAX_PATH);
        std::string path(buffer);
        size_t lastSlash = path.find_last_of("\\/");
        return path.substr(0, lastSlash) + "\\ManhuntHud.SA\\";
    }

    ManhuntHud() {
        // 1. Load the Texture when RenderWare initializes
        Events::initRwEvent += [] {
            std::string modPath = GetModFolder();
            int txdSlot = CTxdStore::AddTxdSlot("mhud");

            // Load the TXD file from the mod folder
            if (CTxdStore::LoadTxd(txdSlot, (modPath + "MHud.TXD").c_str())) {
                CTxdStore::AddRef(txdSlot);
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(txdSlot);

                // Set the sprite to use the "mh" texture entry
                mhSprite.SetTexture((char*)"mh");

                CTxdStore::PopCurrentTxd();
            }
            };

        // 2. Render the texture every frame over the HUD
        Events::drawHudEvent += [] {
            if (mhSprite.m_pTexture) {
                // Fetch current screen resolution
                float screenW = (float)RsGlobal.maximumWidth;
                float screenH = (float)RsGlobal.maximumHeight;

                // Draw full-screen (0,0 to width,height)
                // Color is White (255,255,255) with full Alpha (255)
                mhSprite.Draw(0.0f, 0.0f, screenW, screenH, CRGBA(255, 255, 255, 255));
            }
            };

        // 3. Clean up memory on exit
        Events::shutdownRwEvent += [] {
            mhSprite.Delete();
            };
    }
} manhuntHud;