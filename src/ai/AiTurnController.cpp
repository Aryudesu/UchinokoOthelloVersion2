#include "ai/AiTurnController.h"

#include <utility>

AiTurnController::AiTurnController()
    : delayRandom_(std::random_device{}()) {
}

AiTurnController::~AiTurnController() {
    Cancel();
}

void AiTurnController::Configure(const DifficultyProfile& profile) {
    Cancel();

    ai_.setDepth(profile.searchDepth);
    ai_.setExactEndgameEmpty(profile.exactEndgameEmpty);
    ai_.configureNeuralOrdering(
        profile.neuralOrderingEnabled,
        profile.neuralModelPath,
        profile.neuralOrderingMinimumDepth
    );
    minimumThinkingMs_ = profile.minimumThinkingMs;
    maximumThinkingMs_ = profile.maximumThinkingMs;
    searchTimeLimit_ = std::chrono::milliseconds(profile.timeLimitMs);
}

void AiTurnController::Start(const BitBoard& board, Disc aiDisc) {
    Cancel();

    progress_.reset(0, false);
    searchFinished_.store(false, std::memory_order_relaxed);
    timeoutRequested_ = false;
    completed_ = false;
    completedMove_.reset();

    std::uniform_int_distribution<int> delayDistribution(
        minimumThinkingMs_,
        maximumThinkingMs_
    );
    selectedMinimumTime_ = std::chrono::milliseconds(
        delayDistribution(delayRandom_)
    );
    startedAt_ = std::chrono::steady_clock::now();
    active_ = true;

    thread_ = std::jthread(
        [this, board, aiDisc](std::stop_token stopToken) {
            auto result = ai_.chooseMove(
                board,
                aiDisc,
                stopToken,
                &progress_
            );
            {
                std::lock_guard lock(resultMutex_);
                pendingMove_ = std::move(result);
            }
            searchFinished_.store(true, std::memory_order_release);
        }
    );
}

void AiTurnController::Update() {
    if (!active_) return;

    const auto elapsed =
        std::chrono::steady_clock::now() - startedAt_;

    if (
        !searchFinished_.load(std::memory_order_acquire) &&
        !timeoutRequested_ &&
        elapsed >= searchTimeLimit_
    ) {
        timeoutRequested_ = true;
        if (thread_.joinable()) thread_.request_stop();
    }

    if (
        !searchFinished_.load(std::memory_order_acquire) ||
        elapsed < selectedMinimumTime_
    ) {
        return;
    }

    if (thread_.joinable()) thread_.join();
    {
        std::lock_guard lock(resultMutex_);
        completedMove_ = std::move(pendingMove_);
        pendingMove_.reset();
    }
    active_ = false;
    completed_ = true;
}

void AiTurnController::Cancel() {
    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
    }

    active_ = false;
    completed_ = false;
    timeoutRequested_ = false;
    searchFinished_.store(false, std::memory_order_relaxed);
    progress_.reset(0, false);

    std::lock_guard lock(resultMutex_);
    pendingMove_.reset();
    completedMove_.reset();
}

std::optional<OthelloAI::Move> AiTurnController::TakeMove() {
    if (!completed_) return std::nullopt;

    completed_ = false;
    auto result = std::move(completedMove_);
    completedMove_.reset();
    return result;
}

long long AiTurnController::ElapsedMilliseconds() const noexcept {
    if (!active_) return 0;
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt_
    ).count();
}
