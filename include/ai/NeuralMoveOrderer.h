#pragma once

#include "ai/inference/FastInferenceModel.h"
#include "model/BitBoard.h"

#include <array>
#include <cstddef>
#include <string>

class NeuralMoveOrderer {
public:
    static constexpr std::size_t InputSize = 128;
    static constexpr std::size_t OutputSize = 64;

    bool Configure(
        bool enabled,
        const std::string& modelPath,
        int minimumDepth
    );

    [[nodiscard]] bool Score(
        const BitBoard& board,
        Disc turn,
        int remainingDepth,
        std::array<float, OutputSize>& scores
    ) noexcept;

    [[nodiscard]] bool IsActive() const noexcept { return active_; }

private:
    FastInferenceModel model_;
    std::array<float, InputSize> input_{};
    int minimumDepth_ = 4;
    bool active_ = false;
};
