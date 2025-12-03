#include "util/Dialog.h"
#include <windows.h>

// 共通処理
static inline void ShowMessageBox(
    const std::string& msg,
    const std::string& title,
    UINT type
) {
    MessageBoxA(
        GetActiveWindow(),
        msg.c_str(),
        title.c_str(),
        type
    );
}

// OK ダイアログ（情報用）
void ShowInfo(const std::string& msg, const std::string& title) {
    ShowMessageBox(msg, title, MB_OK | MB_ICONINFORMATION);
}

// エラー用
void ShowError(const std::string& msg, const std::string& title) {
    ShowMessageBox(msg, title, MB_OK | MB_ICONERROR);
}

// Yes/No（確認ダイアログ）
bool AskYesNo(const std::string& msg, const std::string& title) {
    int res = MessageBoxA(
        GetActiveWindow(),
        msg.c_str(),
        title.c_str(),
        MB_YESNO | MB_ICONQUESTION
    );
    return res == IDYES;
}
