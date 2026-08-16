#pragma once

#include "model/BitBoard.h"

#include <cstdint>
#include <optional>
#include <vector>

class OthelloAI {
public:
    struct Move {
        int row = -1;
        int col = -1;
        int score = 0;
        std::uint64_t searchedNodes = 0;
        int searchDepth = 0;
        bool exactSearch = false;
    };

    explicit OthelloAI(int depth = 5) noexcept;

    void setDepth(int depth) noexcept;
    void setExactEndgameEmpty(int emptyCount) noexcept;
    [[nodiscard]] int depth() const noexcept { return depth_; }
    [[nodiscard]] int exactEndgameEmpty() const noexcept {
        return exactEndgameEmpty_;
    }

    [[nodiscard]] std::optional<Move> chooseMove(
        const BitBoard& board,
        Disc disc
    ) const;

private:
    static constexpr int Infinity = 2'000'000;
    static constexpr int WinScore = 1'000'000;

    int depth_ = 5;
    int exactEndgameEmpty_ = 14;
    mutable std::uint64_t searchedNodes_ = 0;

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
        Disc disc
    ) const;

    [[nodiscard]] static Disc opponentOf(Disc disc) noexcept;
};
