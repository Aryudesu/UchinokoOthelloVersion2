#pragma once

#include "model/BitBoard.h"

#include <cstdint>
#include <string>

struct SearchStatisticsEntry {
    std::string difficultyName;
    BitBoard::Bits black = 0;
    BitBoard::Bits white = 0;
    Disc turn = Disc::Empty;
    int configuredDepth = 0;
    int targetDepth = 0;
    int completedDepth = 0;
    int emptyCount = 0;
    int legalMoveCount = 0;
    int timeLimitMs = 0;
    long long elapsedMs = 0;
    std::uint64_t searchedNodes = 0;
    std::uint64_t transpositionHits = 0;
    bool exactSearch = false;
    bool timedOut = false;
    bool neuralOrderingEnabled = false;
    bool neuralOrderingActive = false;
    int neuralOrderingMinimumDepth = 0;
    int moveRow = -1;
    int moveCol = -1;
    int score = 0;
    bool openingBook = false;
};

class SearchStatisticsLogger {
public:
    [[nodiscard]] static bool Append(
        const std::string& path,
        const SearchStatisticsEntry& entry
    ) noexcept;
};
