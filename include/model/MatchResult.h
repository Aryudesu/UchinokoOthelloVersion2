#pragma once

#include "model/BitBoard.h"

#include <cstdlib>

enum class MatchWinner {
    Black,
    White,
    Draw,
};

struct MatchResult {
    int blackCount = 0;
    int whiteCount = 0;
    int difference = 0;
    MatchWinner winner = MatchWinner::Draw;

    [[nodiscard]] static MatchResult From(const BitBoard& board) noexcept {
        MatchResult result;
        result.blackCount = board.count(Disc::Black);
        result.whiteCount = board.count(Disc::White);
        result.difference = std::abs(result.blackCount - result.whiteCount);

        if (result.blackCount > result.whiteCount) {
            result.winner = MatchWinner::Black;
        } else if (result.whiteCount > result.blackCount) {
            result.winner = MatchWinner::White;
        }
        return result;
    }
};
