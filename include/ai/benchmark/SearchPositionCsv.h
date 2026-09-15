#pragma once
#include "model/BitBoard.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace SearchBenchmark {
    struct Position {
        BitBoard board;
        Disc turn = Disc::Empty;
        std::string sourcePath;
        std::size_t sourceRow = 0;
    };
    struct GenerationOptions {
        std::size_t count = 0;
        int minimumEmpty = 17;
        int maximumEmpty = 43;
        std::uint32_t seed = 1'592'593U;
    };
    [[nodiscard]] bool LoadPositions(
        const std::string& path,
        std::vector<Position>& positions,
        std::string& error
    );
    [[nodiscard]] bool GeneratePositions(
        const GenerationOptions& options,
        std::vector<Position>& positions,
        std::string& error
    );
}
