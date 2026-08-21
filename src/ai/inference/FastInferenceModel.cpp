#include "ai/inference/FastInferenceModel.h"
#include "ai/inference/ModelFormat.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <utility>

namespace {
    constexpr std::uint32_t MaximumParameterCount = 1024;
    constexpr std::size_t MaximumMatrixElements = 64 * 1024 * 1024;

    struct SerializedMatrix {
        std::size_t rows = 0;
        std::size_t columns = 0;
        std::vector<float> values;
    };

    bool readMatrix(std::ifstream& input, SerializedMatrix& matrix) {
        std::int32_t rows = 0;
        std::int32_t columns = 0;
        input.read(reinterpret_cast<char*>(&rows), sizeof(rows));
        input.read(reinterpret_cast<char*>(&columns), sizeof(columns));
        if (!input || rows <= 0 || columns <= 0) return false;

        const auto rowCount = static_cast<std::size_t>(rows);
        const auto columnCount = static_cast<std::size_t>(columns);
        if (rowCount > MaximumMatrixElements / columnCount) return false;

        matrix.rows = rowCount;
        matrix.columns = columnCount;
        matrix.values.resize(rowCount * columnCount);
        input.read(
            reinterpret_cast<char*>(matrix.values.data()),
            static_cast<std::streamsize>(
                matrix.values.size() * sizeof(float)
            )
        );
        return input.good();
    }
}

bool FastInferenceModel::load(const std::string& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) return false;

    char magic[sizeof(ModelFormat::Magic) - 1]{};
    std::uint32_t version = 0;
    std::uint32_t parameterCount = 0;
    input.read(magic, sizeof(magic));
    input.read(reinterpret_cast<char*>(&version), sizeof(version));
    input.read(
        reinterpret_cast<char*>(&parameterCount),
        sizeof(parameterCount)
    );
    if (
        !input ||
        std::string(magic, sizeof(magic)) != ModelFormat::Magic ||
        version != ModelFormat::Version ||
        parameterCount < 2 ||
        (parameterCount & 1U) != 0 ||
        parameterCount > MaximumParameterCount
    ) {
        return false;
    }

    std::vector<SerializedMatrix> matrices(parameterCount);
    for (auto& matrix : matrices) {
        if (!readMatrix(input, matrix)) return false;
    }

    std::vector<DenseLayer> loadedLayers;
    loadedLayers.reserve(parameterCount / 2);
    std::size_t maximumWidth = 0;
    std::size_t previousOutputSize = 0;
    for (std::size_t i = 0; i < matrices.size(); i += 2) {
        auto& weight = matrices[i];
        auto& bias = matrices[i + 1];
        if (
            bias.rows != weight.rows ||
            bias.columns != 1 ||
            (i != 0 && weight.columns != previousOutputSize)
        ) {
            return false;
        }

        DenseLayer layer;
        layer.inputSize = weight.columns;
        layer.outputSize = weight.rows;
        layer.weights = std::move(weight.values);
        layer.biases = std::move(bias.values);
        maximumWidth = std::max(maximumWidth, layer.outputSize);
        previousOutputSize = layer.outputSize;
        loadedLayers.push_back(std::move(layer));
    }

    std::vector<float> workspaceA(maximumWidth);
    std::vector<float> workspaceB(maximumWidth);
    layers_.swap(loadedLayers);
    workspaceA_.swap(workspaceA);
    workspaceB_.swap(workspaceB);
    return true;
}

bool FastInferenceModel::isLoaded() const noexcept {
    return !layers_.empty();
}

std::size_t FastInferenceModel::inputSize() const noexcept {
    return layers_.empty() ? 0 : layers_.front().inputSize;
}

std::size_t FastInferenceModel::outputSize() const noexcept {
    return layers_.empty() ? 0 : layers_.back().outputSize;
}

bool FastInferenceModel::predict(
    const float* input,
    std::size_t inputCount,
    float* output,
    std::size_t outputCount
) noexcept {
    if (
        !input ||
        !output ||
        layers_.empty() ||
        inputCount != inputSize() ||
        outputCount < outputSize()
    ) {
        return false;
    }

    const float* currentInput = input;
    std::size_t currentSize = inputCount;
    bool useFirstWorkspace = true;
    for (
        std::size_t layerIndex = 0;
        layerIndex < layers_.size();
        ++layerIndex
    ) {
        const DenseLayer& layer = layers_[layerIndex];
        if (layer.inputSize != currentSize) return false;

        const bool isOutputLayer =
            layerIndex + 1 == layers_.size();
        float* currentOutput = isOutputLayer
            ? output
            : (useFirstWorkspace
                ? workspaceA_.data()
                : workspaceB_.data());

        for (
            std::size_t outputIndex = 0;
            outputIndex < layer.outputSize;
            ++outputIndex
        ) {
            float value = layer.biases[outputIndex];
            const float* weights =
                layer.weights.data() +
                outputIndex * layer.inputSize;
            for (
                std::size_t inputIndex = 0;
                inputIndex < layer.inputSize;
                ++inputIndex
            ) {
                value +=
                    weights[inputIndex] * currentInput[inputIndex];
            }
            if (!isOutputLayer && value < 0.0f) value = 0.0f;
            currentOutput[outputIndex] = value;
        }

        currentInput = currentOutput;
        currentSize = layer.outputSize;
        if (!isOutputLayer) useFirstWorkspace = !useFirstWorkspace;
    }
    return true;
}
