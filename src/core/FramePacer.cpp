#include "core/FramePacer.h"

FramePacer::FramePacer()
    : mStartMs(0)
    , mCount(0)
    , mFps(0.0f)
{
}

// 1フレームの先頭で呼ぶ
void FramePacer::Update() {
    if (mCount == 0) {
        // サンプル開始のタイミング
        mStartMs = GetNowCount();
    }

    ++mCount;

    // 一定フレームごとに実測FPSを更新
    if (mCount >= kSampleFrames) {
        std::uint32_t now = GetNowCount();
        std::uint32_t diff = now - mStartMs;
        if (diff > 0) {
            // diff[ms] かかったkSampleFramesフレームからFPSを計算
            mFps = (1000.0f * kSampleFrames) / static_cast<float>(diff);
        }
        else {
            mFps = 0.0f;
        }
        // 次のサンプルのためにリセット
        mStartMs = now;
        mCount = 0;
    }
}

// 1フレームの最後で呼ぶ
void FramePacer::Wait() {
    // サンプル開始からここまでにかかった時間
    std::uint32_t now = GetNowCount();
    std::uint32_t elapsed = now - mStartMs;

    // 今のフレーム数が理想的には何ms進んでいてほしいか
    int targetMs = mCount * 1000 / kTargetFps;

    int waitMs = targetMs - static_cast<int>(elapsed);
    if (waitMs > 0) {
        Sleep(waitMs);
    }
}
