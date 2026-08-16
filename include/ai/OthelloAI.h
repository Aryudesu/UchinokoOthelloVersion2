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
    };

    explicit OthelloAI(int depth = 5) noexcept;

    void setDepth(int depth) noexcept;
    [[nodiscard]] int depth() const noexcept { return depth_; }

    [[nodiscard]] std::optional<Move> chooseMove(
        const BitBoard& board,
        Disc disc
    ) const;

private:
    static constexpr int Infinity = 2'000'000;
    static constexpr int WinScore = 1'000'000;

    int depth_ = 5;
    mutable std::uint64_t searchedNodes_ = 0;

    [[nodiscard]] int negamax(
        const BitBoard& board,
        Disc turn,
        int depth,
        int alpha,
        int beta
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
