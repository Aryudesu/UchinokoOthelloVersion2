#include "FramePacer.h"

FramePacer::FramePacer()
    : mStartMs(0), mCount(0), mFps(0.0f) {
}

bool FramePacer::Update() {
    if (mCount == 0) {
        // ブロック先頭フレームで基準時刻を記録
        mStartMs = static_cast<std::uint32_t>(GetNowCount());
    }
    if (mCount == kSampleFrames) {
        const std::uint32_t t = static_cast<std::uint32_t>(GetNowCount());
        const float elapsedMs = static_cast<float>(t - mStartMs);
        // 60フレームぶんの平均FPS
        if (elapsedMs > 0.0f) {
            mFps = 1000.0f * kSampleFrames / elapsedMs;
        }
        mCount = 0;
        mStartMs = t;
    }
    ++mCount;
    return true;
}

void FramePacer::Wait() {
    // ブロック開始からの経過時間
    const std::uint32_t now = static_cast<std::uint32_t>(GetNowCount());
    const std::uint32_t tookMs = now - mStartMs;
    const std::uint32_t idealMs = static_cast<std::uint32_t>(mCount * 1000 / kTargetFps);

    if (idealMs > tookMs) {
        // 1ms手前までWaitTimer、最後は軽く回す
        const std::uint32_t grossWait = idealMs - tookMs;
        if (grossWait > 1) {
            WaitTimer(static_cast<int>(grossWait - 1));
        }
        while (static_cast<std::uint32_t>(GetNowCount() - mStartMs) < idealMs) {}
    }
}
