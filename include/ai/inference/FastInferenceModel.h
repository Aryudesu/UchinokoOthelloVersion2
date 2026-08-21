#pragma once

#include <cstddef>
#include <string>
#include <vector>

class FastInferenceModel {
public:
    bool load(const std::string& filePath);

    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] std::size_t inputSize() const noexcept;
    [[nodiscard]] std::size_t outputSize() const noexcept;

    bool predict(
        const float* input,
        std::size_t inputCount,
        float* output,
        std::size_t outputCount
    ) noexcept;

private:
    struct DenseLayer {
        std::size_t inputSize = 0;
        std::size_t outputSize = 0;
        std::vector<float> weights;
        std::vector<float> biases;
    };

    std::vector<DenseLayer> layers_;
    std::vector<float> workspaceA_;
    std::vector<float> workspaceB_;
};
