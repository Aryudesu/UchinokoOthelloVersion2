#pragma once
#include "DxLib.h"
#include <algorithm>
#include <cstdint>

class FramePacer {
public:
    static constexpr int kSampleFrames = 60; // 平均を取るサンプル数
    static constexpr int kTargetFps = 60; // 目標FPS

    // シングルトンインスタンス取得
    static FramePacer& GetInstance() {
        static FramePacer inst;
        return inst;
    }

    // 1フレームの最初に呼ぶ（計測を進める）
    void Update();

    // 1フレームの最後に呼ぶ（目標FPSまで待機）
    void Wait();

    // 現在の推定FPSを取得（kSampleFramesごとに更新）
    float GetFps() const { return mFps; }

private:
    FramePacer();
    FramePacer(const FramePacer&) = delete;
	FramePacer& operator=(const FramePacer&) = delete;

    std::uint32_t mStartMs;     // 計測開始時刻（ms）
    int           mCount;       // 経過フレーム数
    float         mFps;         // 移動平均FPS
};
