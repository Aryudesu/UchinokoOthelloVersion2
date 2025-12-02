#include "DxLib.h"
#include "SceneManager.h"
#include "Ids.h"
#include "InputManager.h"
#include "FramePacer.h"
#include "Logger.h"
#include <exception>
#include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    Logger::I().init("logs/game.log", 3 * 1024 * 1024, Logger::Level::Debug);
    SceneManager mgr;
    mgr.startWith(SceneID::Title);
    try {
        ChangeWindowMode(TRUE), DxLib_Init(), SetDrawScreen(DX_SCREEN_BACK);
        while (ScreenFlip() == 0 && ProcessMessage() == 0 && ClearDrawScreen() == 0) {
			FramePacer::GetInstance().Update();
			InputManager::GetInstance().Update();
            mgr.updateAndDraw();
			FramePacer::GetInstance().Wait();
        }
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Error", MB_OK | MB_ICONERROR);
    }
    DxLib_End(); // DXÉâÉCÉuÉâÉäèIóπèàóù
    return 0;
}
