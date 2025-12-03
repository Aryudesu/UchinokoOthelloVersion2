#pragma once
#include "util/Log.h"
#include <DxLib.h>

class DebugOverlay {
public:
    DebugOverlay(int toggleKey = KEY_INPUT_F1)
        : toggleKey_(toggleKey) {
    }

    // –ˆƒtƒŒ[ƒ€ŒÄ‚Ô
    void updateAndDraw();

private:
    int toggleKey_;
    bool visible_ = false;
    bool prevKey_ = false;
};
