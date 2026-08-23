#include "ai/NeuralMoveOrderer.h"

#include <algorithm>
#include <cmath>
#include <utility>

bool NeuralMoveOrderer::Configure(
    bool enabled,
    const std::string& modelPath,
    int minimumDepth
) {
    active_ = false;
    minimumDepth_ = std::clamp(minimumDepth, 1, 64);
    if (!enabled) return false;

    FastInferenceModel candidate;
    if (
        !candidate.load(modelPath) ||
        candidate.inputSize() != InputSize ||
        candidate.outputSize() != OutputSize
    ) {
        return false;
    }

    model_ = std::move(candidate);
    active_ = true;
    return true;
}

bool NeuralMoveOrderer::Score(
    const BitBoard& board,
    Disc turn,
    int remainingDepth,
    std::array<float, OutputSize>& scores
) noexcept {
    if (
        !active_ ||
        turn == Disc::Empty ||
        remainingDepth < minimumDepth_
    ) {
        return false;
    }

    input_.fill(0.0f);
    const BitBoard::Bits mine =
        turn == Disc::Black ? board.black() : board.white();
    const BitBoard::Bits theirs =
        turn == Disc::Black ? board.white() : board.black();

    for (std::size_t index = 0; index < OutputSize; ++index) {
        const auto bit =
            static_cast<BitBoard::Bits>(1) << index;
        if ((mine & bit) != 0) input_[index] = 1.0f;
        if ((theirs & bit) != 0) {
            input_[OutputSize + index] = 1.0f;
        }
    }

    if (
        !model_.predict(
            input_.data(),
            input_.size(),
            scores.data(),
            scores.size()
        )
    ) {
        return false;
    }

    return std::all_of(
        scores.begin(),
        scores.end(),
        [](float value) { return std::isfinite(value); }
    );
}
