#pragma once
#include "DxLib.h"
#include "Singleton.h"
#include <algorithm>
#include <cstdint>

class FramePacer : public Singleton<FramePacer> {
    std::uint32_t mStartMs;     // 計測開始時刻（ms）
    int           mCount;       // 経過フレーム数
    float         mFps;         // 移動平均FPS

public:
    static constexpr int kSampleFrames = 60; // 平均を取るサンプル数
    static constexpr int kTargetFps = 60; // 目標FPS

    FramePacer();

    // 1フレームの最初に呼ぶ（計測を進める）
    bool Update();

    // 1フレームの最後に呼ぶ（目標FPSまで待機）
    void Wait();

    // 現在の推定FPSを取得（kSampleFramesごとに更新）
    float GetFps() const { return mFps; }
};
