#pragma once

#include "ai/OthelloAI.h"
#include "core/GameSettings.h"
#include "model/BitBoard.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <random>
#include <string_view>
#include <thread>

class AiTurnController {
public:
    AiTurnController();
    ~AiTurnController();

    AiTurnController(const AiTurnController&) = delete;
    AiTurnController& operator=(const AiTurnController&) = delete;

    void Configure(const DifficultyProfile& profile);
    void Start(const BitBoard& board, Disc aiDisc);
    void Update();
    void Cancel();

    [[nodiscard]] bool IsActive() const noexcept { return active_; }
    [[nodiscard]] bool HasCompleted() const noexcept { return completed_; }
    [[nodiscard]] std::optional<OthelloAI::Move> TakeMove();

    [[nodiscard]] const OthelloAI::SearchProgress& Progress() const noexcept {
        return progress_;
    }
    [[nodiscard]] int ConfiguredDepth() const noexcept {
        return ai_.depth();
    }
    [[nodiscard]] long long ElapsedMilliseconds() const noexcept;
    [[nodiscard]] std::u8string_view CompletedOpeningName(
        const BitBoard& board,
        Disc turn
    ) const noexcept {
        return ai_.completedOpeningName(board, turn);
    }

private:
    OthelloAI ai_;
    OthelloAI::SearchProgress progress_;
    std::optional<OthelloAI::Move> pendingMove_;
    std::optional<OthelloAI::Move> completedMove_;
    std::mutex resultMutex_;
    std::atomic<bool> searchFinished_{ false };
    std::mt19937 delayRandom_;

    int minimumThinkingMs_ = 600;
    int maximumThinkingMs_ = 800;
    std::chrono::milliseconds selectedMinimumTime_{ 700 };
    std::chrono::milliseconds searchTimeLimit_{ 10'000 };
    std::chrono::steady_clock::time_point startedAt_{};

    bool active_ = false;
    bool completed_ = false;
    bool timeoutRequested_ = false;
    std::jthread thread_;
};
