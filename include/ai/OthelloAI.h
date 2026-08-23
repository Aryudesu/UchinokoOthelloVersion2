#pragma once

#include "model/BitBoard.h"
#include "ai/NeuralMoveOrderer.h"
#include "ai/OpeningBook.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <stop_token>
#include <string>
#include <unordered_map>
#include <vector>

class OthelloAI {
public:
    struct SearchProgress {
        std::atomic<std::uint64_t> searchedNodes{ 0 };
        std::atomic<std::uint64_t> transpositionHits{ 0 };
        std::atomic<int> completedDepth{ 0 };
        std::atomic<int> targetDepth{ 0 };
        std::atomic<bool> exactSearch{ false };
        std::atomic<bool> timedOut{ false };

        void reset(int target, bool exact) noexcept;
    };

    struct Move {
        int row = -1;
        int col = -1;
        int score = 0;
        std::uint64_t searchedNodes = 0;
        int searchDepth = 0;
        bool exactSearch = false;
        std::uint64_t transpositionHits = 0;
        int completedIterations = 0;
        bool openingBook = false;
    };

    explicit OthelloAI(int depth = 5) noexcept;

    void setDepth(int depth) noexcept;
    void setExactEndgameEmpty(int emptyCount) noexcept;
    bool configureNeuralOrdering(
        bool enabled,
        const std::string& modelPath,
        int minimumDepth
    );
    [[nodiscard]] bool neuralOrderingActive() const noexcept {
        return neuralMoveOrderer_.IsActive();
    }
    [[nodiscard]] int depth() const noexcept { return depth_; }
    [[nodiscard]] int exactEndgameEmpty() const noexcept {
        return exactEndgameEmpty_;
    }

    [[nodiscard]] std::u8string_view completedOpeningName(
        const BitBoard& board,
        Disc turn
    ) const noexcept {
        return openingBook_.completedName(board, turn);
    }

    [[nodiscard]] std::optional<Move> chooseMove(
        const BitBoard& board,
        Disc disc,
        std::stop_token stopToken = {},
        SearchProgress* progress = nullptr,
        std::optional<std::chrono::steady_clock::time_point> deadline =
            std::nullopt
    ) const;

private:
    static constexpr int Infinity = 2'000'000;
    static constexpr int WinScore = 1'000'000;
    static constexpr std::size_t MaxTranspositionEntries = 500'000;

    enum class Bound : std::uint8_t {
        Exact,
        Lower,
        Upper,
    };

    struct PositionKey {
        BitBoard::Bits black = 0;
        BitBoard::Bits white = 0;
        Disc turn = Disc::Empty;

        bool operator==(const PositionKey&) const noexcept = default;
    };

    struct PositionKeyHash {
        [[nodiscard]] std::size_t operator()(
            const PositionKey& key
        ) const noexcept;
    };

    struct TranspositionEntry {
        int depth = -1;
        int score = 0;
        int bestMoveIndex = -1;
        Bound bound = Bound::Exact;
    };

    int depth_ = 5;
    OpeningBook openingBook_;
    mutable NeuralMoveOrderer neuralMoveOrderer_;
    int exactEndgameEmpty_ = 14;
    mutable std::uint64_t searchedNodes_ = 0;
    mutable std::uint64_t transpositionHits_ = 0;
    mutable std::stop_token stopToken_;
    mutable std::optional<std::chrono::steady_clock::time_point> deadline_;
    mutable SearchProgress* progress_ = nullptr;
    mutable std::unordered_map<
        PositionKey,
        TranspositionEntry,
        PositionKeyHash
    > transpositionTable_;

    [[nodiscard]] Move searchRoot(
        const BitBoard& board,
        Disc disc,
        int depth,
        bool exactSearch,
        int preferredMoveIndex
    ) const;

    [[nodiscard]] int negaScout(
        const BitBoard& board,
        Disc turn,
        int depth,
        int alpha,
        int beta,
        bool exactSearch
    ) const;

    [[nodiscard]] int evaluate(const BitBoard& board, Disc perspective) const;
    [[nodiscard]] int terminalScore(
        const BitBoard& board,
        Disc perspective
    ) const;

    [[nodiscard]] std::vector<Move> orderedMoves(
        const BitBoard& board,
        Disc disc,
        int preferredMoveIndex,
        int remainingDepth
    ) const;

    void storeTransposition(
        const PositionKey& key,
        const TranspositionEntry& entry
    ) const;

    void checkCancellation() const;
    void publishProgress() const noexcept;

    [[nodiscard]] static Disc opponentOf(Disc disc) noexcept;
};
