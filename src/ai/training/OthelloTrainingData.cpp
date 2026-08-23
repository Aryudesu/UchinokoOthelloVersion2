#include "ai/training/OthelloTrainingData.h"

#include <algorithm>

namespace OthelloTrainingData {
int TransformIndex(int index, int symmetry) noexcept {
    if (index < 0 || index >= static_cast<int>(OutputSize)) return -1;

    int row = index / BitBoard::Size;
    int col = index % BitBoard::Size;
    const int normalizedSymmetry = std::clamp(
        symmetry,
        0,
        SymmetryCount - 1
    );

    if (normalizedSymmetry >= 4) {
        col = BitBoard::Size - 1 - col;
    }
    for (int rotation = 0; rotation < normalizedSymmetry % 4; ++rotation) {
        const int nextRow = col;
        const int nextCol = BitBoard::Size - 1 - row;
        row = nextRow;
        col = nextCol;
    }
    return row * BitBoard::Size + col;
}

Input Encode(
    const BitBoard& board,
    Disc turn,
    int symmetry
) noexcept {
    Input result{};
    if (turn == Disc::Empty) return result;

    const BitBoard::Bits mine =
        turn == Disc::Black ? board.black() : board.white();
    const BitBoard::Bits theirs =
        turn == Disc::Black ? board.white() : board.black();

    for (int index = 0; index < static_cast<int>(OutputSize); ++index) {
        const int transformed = TransformIndex(index, symmetry);
        const BitBoard::Bits bit =
            static_cast<BitBoard::Bits>(1) << index;
        if ((mine & bit) != 0) {
            result[static_cast<std::size_t>(transformed)] = 1.0f;
        }
        if ((theirs & bit) != 0) {
            result[OutputSize + static_cast<std::size_t>(transformed)] = 1.0f;
        }
    }
    return result;
}

bool CsvWriter::Open(const std::string& path) {
    output_.close();
    output_.clear();
    output_.open(path, std::ios::out | std::ios::trunc);
    rowsWritten_ = 0;
    if (!output_) return false;

    output_ << "# uchinoko_othello_move_ordering_v1\n"
            << "# input_size=128\n"
            << "# output_size=64\n";
    return output_.good();
}

bool CsvWriter::Write(
    const BitBoard& board,
    Disc turn,
    int bestMoveIndex,
    bool augmentSymmetries
) {
    if (
        !output_ ||
        turn == Disc::Empty ||
        bestMoveIndex < 0 ||
        bestMoveIndex >= static_cast<int>(OutputSize) ||
        !board.canPut(
            turn,
            bestMoveIndex / BitBoard::Size,
            bestMoveIndex % BitBoard::Size
        )
    ) {
        return false;
    }

    const int symmetryCount = augmentSymmetries ? SymmetryCount : 1;
    for (int symmetry = 0; symmetry < symmetryCount; ++symmetry) {
        const Input input = Encode(board, turn, symmetry);
        for (const float value : input) {
            output_ << static_cast<int>(value) << ',';
        }
        output_ << TransformIndex(bestMoveIndex, symmetry) << '\n';
        if (!output_) return false;
        ++rowsWritten_;
    }
    return true;
}
}
