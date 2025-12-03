#include "util/DebugOverlay.h"

void DebugOverlay::updateAndDraw() {
    int nowKey = CheckHitKey(toggleKey_);

    // F1 押しっぱなし判定を避けてトグル
    if (nowKey && !prevKey_) {
        visible_ = !visible_;
    }
    prevKey_ = nowKey;

    if (!visible_) return;

    auto entries = Log::I().latestEntries();

    const int x = 10;
    const int y = 10;
    const int w = 1200;
    const int h = 300;

    int bg = GetColor(0, 0, 0);
    int border = GetColor(80, 80, 80);
    DrawBoxAA((float)x, (float)y, (float)(x + w), (float)(y + h), bg, TRUE);
    DrawBoxAA((float)x, (float)y, (float)(x + w), (float)(y + h), border, FALSE);

    int headColor = GetColor(180, 180, 255);
    DrawString(x + 8, y + 8, "LOG (F1: toggle)", headColor);

    int yy = y + 28;
    const int lineH = 16;

    // 新しいログを下側に表示
    for (int i = (int)entries.size() - 1; i >= 0 && yy + lineH < y + h - 4; --i) {
        const auto& e = entries[i];

        int color = GetColor(220, 220, 220);
        if (e.level == Log::Level::Warn) {
            color = GetColor(255, 220, 160);
        }
        else if (e.level == Log::Level::Error) {
            color = GetColor(255, 150, 150);
        }

        // 長すぎる行はカット
        std::string s = e.text.size() > 150 ? (e.text.substr(0, 150) + "...") : e.text;
        DrawString(x + 8, yy, s.c_str(), color);
        yy += lineH;
    }
}
