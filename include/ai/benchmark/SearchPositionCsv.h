#pragma once
#include "model/BitBoard.h"
#include <cstddef>
#include <string>
#include <vector>

namespace SearchBenchmark {
    struct Position {
        BitBoard board;
        Disc turn = Disc::Empty;
        std::string sourcePath;
        std::size_t sourceRow = 0;
    };
    [[nodiscard]] bool LoadPositions(
        const std::string& path,
        std::vector<Position>& positions,
        std::string& error
    );
}
