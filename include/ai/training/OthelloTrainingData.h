#pragma once

#include "model/BitBoard.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <string>

namespace OthelloTrainingData {
inline constexpr std::size_t InputSize = 128;
inline constexpr std::size_t OutputSize = 64;
inline constexpr int SymmetryCount = 8;

using Input = std::array<float, InputSize>;

[[nodiscard]] int TransformIndex(int index, int symmetry) noexcept;
[[nodiscard]] Input Encode(
    const BitBoard& board,
    Disc turn,
    int symmetry = 0
) noexcept;

class CsvWriter {
public:
    bool Open(const std::string& path);
    bool Write(
        const BitBoard& board,
        Disc turn,
        int bestMoveIndex,
        bool augmentSymmetries = true
    );

    [[nodiscard]] std::size_t rowsWritten() const noexcept {
        return rowsWritten_;
    }

private:
    std::ofstream output_;
    std::size_t rowsWritten_ = 0;
};
}
