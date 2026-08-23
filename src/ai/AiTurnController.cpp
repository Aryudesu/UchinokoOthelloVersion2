#include "ai/AiTurnController.h"
#include "ai/SearchStatisticsLogger.h"
#include "util/Log.h"

#include <bit>
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
    neuralOrderingActive_ = ai_.configureNeuralOrdering(
        profile.neuralOrderingEnabled,
        profile.neuralModelPath,
        profile.neuralOrderingMinimumDepth
    );
    minimumThinkingMs_ = profile.minimumThinkingMs;
    maximumThinkingMs_ = profile.maximumThinkingMs;
    searchTimeLimit_ = std::chrono::milliseconds(profile.timeLimitMs);
    difficultyName_ = profile.name;
    configuredDepth_ = profile.searchDepth;
    timeLimitMs_ = profile.timeLimitMs;
    neuralOrderingEnabled_ = profile.neuralOrderingEnabled;
    neuralOrderingMinimumDepth_ = profile.neuralOrderingMinimumDepth;
    searchStatisticsEnabled_ = profile.searchStatisticsEnabled;
    searchStatisticsPath_ = profile.searchStatisticsPath;
}

void AiTurnController::Start(const BitBoard& board, Disc aiDisc) {
    Cancel();

    progress_.reset(0, false);
    searchFinished_.store(false, std::memory_order_relaxed);
    searchElapsedMs_.store(0, std::memory_order_relaxed);
    timeoutRequested_ = false;
    completed_ = false;
    completedMove_.reset();
    searchBoard_ = board;
    searchDisc_ = aiDisc;

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
            const auto searchStartedAt = std::chrono::steady_clock::now();
            auto result = ai_.chooseMove(
                board,
                aiDisc,
                stopToken,
                &progress_
            );
            searchElapsedMs_.store(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - searchStartedAt
                ).count(),
                std::memory_order_relaxed
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
    appendSearchStatistics(completedMove_);
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

void AiTurnController::appendSearchStatistics(
    const std::optional<OthelloAI::Move>& move
) const {
    if (!searchStatisticsEnabled_ || searchStatisticsPath_.empty()) return;

    const int blackCount = searchBoard_.count(Disc::Black);
    const int whiteCount = searchBoard_.count(Disc::White);
    SearchStatisticsEntry entry;
    entry.difficultyName = difficultyName_;
    entry.black = searchBoard_.black();
    entry.white = searchBoard_.white();
    entry.turn = searchDisc_;
    entry.configuredDepth = configuredDepth_;
    entry.targetDepth = progress_.targetDepth.load(std::memory_order_relaxed);
    entry.completedDepth = progress_.completedDepth.load(std::memory_order_relaxed);
    entry.emptyCount = 64 - blackCount - whiteCount;
    entry.legalMoveCount = std::popcount(searchBoard_.legalMoves(searchDisc_));
    entry.timeLimitMs = timeLimitMs_;
    entry.elapsedMs = searchElapsedMs_.load(std::memory_order_relaxed);
    entry.searchedNodes = progress_.searchedNodes.load(std::memory_order_relaxed);
    entry.transpositionHits =
        progress_.transpositionHits.load(std::memory_order_relaxed);
    entry.exactSearch = progress_.exactSearch.load(std::memory_order_relaxed);
    entry.timedOut = timeoutRequested_;
    entry.neuralOrderingEnabled = neuralOrderingEnabled_;
    entry.neuralOrderingActive = neuralOrderingActive_;
    entry.neuralOrderingMinimumDepth = neuralOrderingMinimumDepth_;
    if (move.has_value()) {
        entry.completedDepth = move->searchDepth;
        entry.searchedNodes = move->searchedNodes;
        entry.transpositionHits = move->transpositionHits;
        entry.moveRow = move->row;
        entry.moveCol = move->col;
        entry.score = move->score;
        entry.openingBook = move->openingBook;
    }

    if (!SearchStatisticsLogger::Append(searchStatisticsPath_, entry)) {
        LOG_WARN("Could not append AI search statistics: " + searchStatisticsPath_);
    }
}
