#pragma once
#include <string>

// OK ダイアログ（情報用）
void ShowInfo(const std::string& msg, const std::string& title = "Info");

// エラー用
void ShowError(const std::string& msg, const std::string& title = "Error");

// Yes/No（確認ダイアログ）
bool AskYesNo(const std::string& msg, const std::string& title = "Confirm");
