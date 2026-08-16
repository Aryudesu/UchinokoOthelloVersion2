#include "DxLib.h"
#include "core/Ids.h"
#include "core/FramePacer.h"
#include "manager/SceneManager.h"
#include "manager/InputManager.h"
#include "util/Log.h"
#include "util/DebugOverlay.h"
#include <exception>
#include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    // ロガー初期化
    Log::I().init();
    DebugOverlay overlay;


	// DXライブラリ初期化
    SetOutApplicationLogValidFlag(FALSE);
    ChangeWindowMode(TRUE);
    if (DxLib_Init() == -1) {
        MessageBox(NULL, "Dxlib Init Failed", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    SetDrawScreen(DX_SCREEN_BACK);

    // シーン開始
    SceneManager mgr;
    mgr.startWith(SceneID::Title);
    try {
        LOG_DEBUG("Mainloop started");
		// メインループ
        while (mgr.running() && ProcessMessage() == 0) {

            FramePacer::GetInstance().Update();
			InputManager::GetInstance().Update();

			// 更新処理
            mgr.update();

			// 描画処理
            if (ClearDrawScreen() != 0) break;
            mgr.draw();
            overlay.updateAndDraw();
            if (ScreenFlip() != 0) break;
            FramePacer::GetInstance().Wait();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("catch error");
        MessageBoxA(NULL, e.what(), "Error", MB_OK | MB_ICONERROR);
    }

    // DXライブラリ終了処理
    DxLib_End();
    return 0;
}
