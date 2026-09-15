#include "ai/OthelloAI.h"
#include "ai/benchmark/SearchPositionCsv.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
    struct Options {
        std::vector<std::string> inputs;
        std::string output = "neural_ordering_benchmark.csv";
        std::string summary;
        std::string model = "data/model/othello_ordering.model";
        std::vector<int> minimumDepths;
        std::vector<int> minimumLegalMoves;
        std::vector<int> blendPercents;
        int depth = 12;
        int exactEndgameEmpty = 0;
        int timeLimitMs = 10'000;
        int repeats = 3;
        int minimumEmpty = 17;
        int maximumEmpty = 43;
        std::size_t maximumPositions = 10;
        std::size_t generatedPositions = 0;
        int seed = 1'592'593;
        bool autoTune = false;
    };
    struct PositionKey {
        BitBoard::Bits black = 0;
        BitBoard::Bits white = 0;
        Disc turn = Disc::Empty;
        bool operator==(const PositionKey&) const noexcept = default;
    };
    struct PositionKeyHash {
        std::size_t operator()(const PositionKey& key) const noexcept {
            return static_cast<std::size_t>(key.black ^ std::rotl(key.white, 23) ^ (static_cast<std::uint64_t>(key.turn) << 61));
        }
    };
    struct Condition {
        std::string name;
        int minimumDepth = 0;
        int minimumLegalMoves = 0;
        int blendPercent = 0;
        std::unique_ptr<OthelloAI> ai;
    };
    struct ConditionParameters {
        int minimumDepth = 0;
        int minimumLegalMoves = 0;
        int blendPercent = 0;
    };
    struct Result {
        double elapsedMs = 0.0;
        std::uint64_t nodes = 0;
        std::uint64_t transpositionHits = 0;
        std::uint64_t neuralOrderingCalls = 0;
        std::uint64_t neuralOrderingCacheHits = 0;
        int targetDepth = 0;
        int completedDepth = 0;
        bool timedOut = false;
        bool openingBook = false;
        int row = -1;
        int col = -1;
        int score = 0;
    };
    struct Aggregate {
        std::vector<double> comparableElapsedRatios;
        std::vector<double> comparableNodeRatios;
        std::size_t runs = 0;
        std::size_t comparableRuns = 0;
        std::size_t shallowerRuns = 0;
        std::size_t deeperRuns = 0;
        std::size_t timeoutRuns = 0;
        std::size_t sameMoveRuns = 0;
        std::uint64_t neuralOrderingCalls = 0;
        std::uint64_t neuralOrderingCacheHits = 0;
    };

    void printUsage() {
        std::cout <<
            "Fixed-position neural move-ordering benchmark\n\n"
            "Position source (at least one):\n"
            "  --input PATH              Search-statistics CSV (repeatable)\n"
            "  --generate-positions N   Deterministic mixed self-play positions\n\n"
            "Options:\n"
            "  --output PATH             Output CSV (default: neural_ordering_benchmark.csv)\n"
            "  --summary PATH            Aggregate CSV (default: OUTPUT stem + _summary.csv)\n"
            "  --model PATH              Neural model path\n"
            "  --depth N                 Search depth 1-20 (default: 12)\n"
            "  --minimum-depth N         Neural minimum depth (repeatable; default: 10,11,12)\n"
            "  --minimum-legal-moves N   Neural minimum legal moves (repeatable; default: 1)\n"
            "  --blend-percent N         Neural rank weight 1-100 (repeatable; default: 100)\n"
            "  --auto-tune               Use a compact built-in parameter set\n"
            "  --exact-endgame-empty N   Exact-search threshold 0-20 (default: 0)\n"
            "  --time-limit-ms N         Deadline per search (default: 10000)\n"
            "  --repeats N               Repeats per position (default: 3)\n"
            "  --min-empty N             Minimum empty squares (default: 17)\n"
            "  --max-empty N             Maximum empty squares (default: 43)\n"
            "  --max-positions N         Position limit; 0 means unlimited (default: 10)\n"
            "  --seed N                  Generated-position seed (default: 1592593)\n"
            "  --help                     Show this help\n";
    }
    bool parseInteger(const std::string& text, int& value) {
        try {
            std::size_t used = 0;
            value = std::stoi(text, &used);
            return used == text.size();
        } catch (...) { return false; }
    }
    bool parseOptions(int argc, char* argv[], Options& options, std::string& error) {
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--help") { printUsage(); return false; }
            if (argument == "--auto-tune") {
                options.autoTune = true;
                continue;
            }
            if (index + 1 >= argc) { error = "Missing value after " + argument; return false; }
            const std::string value = argv[++index];
            int number = 0;
            if (argument == "--input") options.inputs.push_back(value);
            else if (argument == "--output") options.output = value;
            else if (argument == "--summary") options.summary = value;
            else if (argument == "--model") options.model = value;
            else if (!parseInteger(value, number)) { error = "Invalid integer for " + argument + ": " + value; return false; }
            else if (argument == "--depth") options.depth = number;
            else if (argument == "--minimum-depth") options.minimumDepths.push_back(number);
            else if (argument == "--minimum-legal-moves") options.minimumLegalMoves.push_back(number);
            else if (argument == "--blend-percent") options.blendPercents.push_back(number);
            else if (argument == "--exact-endgame-empty") options.exactEndgameEmpty = number;
            else if (argument == "--time-limit-ms") options.timeLimitMs = number;
            else if (argument == "--repeats") options.repeats = number;
            else if (argument == "--min-empty") options.minimumEmpty = number;
            else if (argument == "--max-empty") options.maximumEmpty = number;
            else if (argument == "--max-positions") {
                if (number < 0) { error = "--max-positions must be at least 0"; return false; }
                options.maximumPositions = static_cast<std::size_t>(number);
            } else if (argument == "--generate-positions") {
                if (number < 1 || number > 10'000) { error = "--generate-positions must be between 1 and 10000"; return false; }
                options.generatedPositions = static_cast<std::size_t>(number);
            } else if (argument == "--seed") {
                if (number < 0) { error = "--seed must be at least 0"; return false; }
                options.seed = number;
            } else { error = "Unknown option: " + argument; return false; }
        }
        if (options.inputs.empty() && options.generatedPositions == 0) {
            error = "At least one --input or --generate-positions is required";
        }
        else if (options.depth < 1 || options.depth > 20) error = "--depth must be between 1 and 20";
        else if (options.exactEndgameEmpty < 0 || options.exactEndgameEmpty > 20) error = "--exact-endgame-empty must be between 0 and 20";
        else if (options.timeLimitMs < 1 || options.timeLimitMs > 60'000) error = "--time-limit-ms must be between 1 and 60000";
        else if (options.repeats < 1 || options.repeats > 100) error = "--repeats must be between 1 and 100";
        else if (options.minimumEmpty < 0 || options.maximumEmpty > 60 || options.minimumEmpty > options.maximumEmpty) error = "Empty-square range must be within 0-60";
        if (!error.empty()) return false;
        if (
            options.autoTune &&
            (
                !options.minimumDepths.empty() ||
                !options.minimumLegalMoves.empty() ||
                !options.blendPercents.empty()
            )
        ) {
            error = "--auto-tune cannot be combined with manual neural parameters";
            return false;
        }
        if (!options.autoTune) {
            if (options.minimumDepths.empty()) options.minimumDepths = { 10, 11, 12 };
            if (options.minimumLegalMoves.empty()) options.minimumLegalMoves = { 1 };
            if (options.blendPercents.empty()) options.blendPercents = { 100 };
        }
        std::sort(options.minimumDepths.begin(), options.minimumDepths.end());
        options.minimumDepths.erase(std::unique(options.minimumDepths.begin(), options.minimumDepths.end()), options.minimumDepths.end());
        std::sort(options.minimumLegalMoves.begin(), options.minimumLegalMoves.end());
        options.minimumLegalMoves.erase(std::unique(options.minimumLegalMoves.begin(), options.minimumLegalMoves.end()), options.minimumLegalMoves.end());
        std::sort(options.blendPercents.begin(), options.blendPercents.end());
        options.blendPercents.erase(std::unique(options.blendPercents.begin(), options.blendPercents.end()), options.blendPercents.end());
        for (const int depth : options.minimumDepths) {
            if (depth < 1 || depth > 64) { error = "--minimum-depth must be between 1 and 64"; return false; }
        }
        for (const int legalMoves : options.minimumLegalMoves) {
            if (legalMoves < 1 || legalMoves > 64) { error = "--minimum-legal-moves must be between 1 and 64"; return false; }
        }
        for (const int blendPercent : options.blendPercents) {
            if (blendPercent < 1 || blendPercent > 100) { error = "--blend-percent must be between 1 and 100"; return false; }
        }
        if (options.summary.empty()) {
            const std::filesystem::path outputPath(options.output);
            options.summary = (
                outputPath.parent_path() /
                (outputPath.stem().string() + "_summary.csv")
            ).string();
        }
        return true;
    }
    std::string csvText(const std::string& value) {
        if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
        std::string escaped = "\"";
        for (const char character : value) { if (character == '"') escaped += '"'; escaped += character; }
        return escaped + '"';
    }
    std::string hexBits(BitBoard::Bits bits) {
        std::ostringstream output;
        output << "0x" << std::hex << std::setw(16) << std::setfill('0') << bits;
        return output.str();
    }
    Result runSearch(OthelloAI& ai, const SearchBenchmark::Position& position, int timeLimitMs) {
        OthelloAI::SearchProgress progress;
        const auto start = std::chrono::steady_clock::now();
        const auto move = ai.chooseMove(position.board, position.turn, {}, &progress, start + std::chrono::milliseconds(timeLimitMs));
        const auto finish = std::chrono::steady_clock::now();
        Result result;
        result.elapsedMs = std::chrono::duration<double, std::milli>(finish - start).count();
        result.nodes = progress.searchedNodes.load(std::memory_order_relaxed);
        result.transpositionHits = progress.transpositionHits.load(std::memory_order_relaxed);
        result.neuralOrderingCalls = progress.neuralOrderingCalls.load(std::memory_order_relaxed);
        result.neuralOrderingCacheHits = progress.neuralOrderingCacheHits.load(std::memory_order_relaxed);
        result.targetDepth = progress.targetDepth.load(std::memory_order_relaxed);
        result.completedDepth = progress.completedDepth.load(std::memory_order_relaxed);
        result.timedOut = progress.timedOut.load(std::memory_order_relaxed);
        if (move) {
            result.openingBook = move->openingBook;
            result.row = move->row; result.col = move->col; result.score = move->score;
            result.nodes = move->searchedNodes;
            result.transpositionHits = move->transpositionHits;
            result.neuralOrderingCalls = move->neuralOrderingCalls;
            result.neuralOrderingCacheHits = move->neuralOrderingCacheHits;
            result.completedDepth = move->completedIterations;
        }
        return result;
    }

    double median(std::vector<double> values) {
        if (values.empty()) return 0.0;
        std::sort(values.begin(), values.end());
        const std::size_t middle = values.size() / 2;
        if ((values.size() & 1U) != 0) return values[middle];
        return (values[middle - 1] + values[middle]) / 2.0;
    }

    double geometricMean(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        double logarithmSum = 0.0;
        for (const double value : values) {
            if (value <= 0.0) return 0.0;
            logarithmSum += std::log(value);
        }
        return std::exp(logarithmSum / static_cast<double>(values.size()));
    }

    std::vector<ConditionParameters> conditionParameters(
        const Options& options
    ) {
        std::vector<ConditionParameters> result;
        const auto add = [&result](int depth, int legalMoves, int blend) {
            const ConditionParameters value{ depth, legalMoves, blend };
            const auto duplicate = std::find_if(
                result.begin(),
                result.end(),
                [&value](const ConditionParameters& existing) {
                    return
                        existing.minimumDepth == value.minimumDepth &&
                        existing.minimumLegalMoves == value.minimumLegalMoves &&
                        existing.blendPercent == value.blendPercent;
                }
            );
            if (duplicate == result.end()) result.push_back(value);
        };

        if (options.autoTune) {
            const int rootOnly = options.depth;
            const int broadest = std::max(
                1,
                (options.depth * 2 + 2) / 3
            );
            const int middle = (rootOnly + broadest) / 2;
            add(rootOnly, 1, 50);
            add(rootOnly, 1, 100);
            add(middle, 3, 50);
            add(middle, 3, 100);
            add(broadest, 3, 50);
            add(broadest, 5, 50);
            add(broadest, 5, 100);
            return result;
        }

        for (const int depth : options.minimumDepths) {
            for (const int legalMoves : options.minimumLegalMoves) {
                for (const int blend : options.blendPercents) {
                    add(depth, legalMoves, blend);
                }
            }
        }
        return result;
    }
}

int main(int argc, char* argv[]) {
    Options options;
    std::string error;
    if (!parseOptions(argc, argv, options, error)) {
        if (!error.empty()) { std::cerr << "ERROR: " << error << "\n\n"; printUsage(); return 1; }
        return 0;
    }
    std::vector<SearchBenchmark::Position> loaded;
    for (const std::string& input : options.inputs) {
        if (!SearchBenchmark::LoadPositions(input, loaded, error)) { std::cerr << "ERROR: " << error << '\n'; return 1; }
    }
    if (!SearchBenchmark::GeneratePositions(
        {
            options.generatedPositions,
            options.minimumEmpty,
            options.maximumEmpty,
            static_cast<std::uint32_t>(options.seed),
        },
        loaded,
        error
    )) {
        std::cerr << "ERROR: " << error << '\n';
        return 1;
    }
    std::vector<SearchBenchmark::Position> positions;
    std::unordered_set<PositionKey, PositionKeyHash> seen;
    for (auto& position : loaded) {
        const int emptyCount = 64 - position.board.count(Disc::Black) - position.board.count(Disc::White);
        if (emptyCount < options.minimumEmpty || emptyCount > options.maximumEmpty) continue;
        const PositionKey key{ position.board.black(), position.board.white(), position.turn };
        if (!seen.insert(key).second) continue;
        positions.push_back(std::move(position));
        if (options.maximumPositions != 0 && positions.size() >= options.maximumPositions) break;
    }
    if (positions.empty()) { std::cerr << "ERROR: No positions matched the requested filters.\n"; return 1; }

    std::vector<Condition> conditions;
    auto baseline = std::make_unique<OthelloAI>(options.depth);
    baseline->setExactEndgameEmpty(options.exactEndgameEmpty);
    conditions.push_back({ "baseline", 0, 0, 0, std::move(baseline) });
    for (const ConditionParameters parameters : conditionParameters(options)) {
        auto ai = std::make_unique<OthelloAI>(options.depth);
        ai->setExactEndgameEmpty(options.exactEndgameEmpty);
        if (!ai->configureNeuralOrdering(true, options.model, parameters.minimumDepth, parameters.minimumLegalMoves, parameters.blendPercent)) { std::cerr << "ERROR: Could not load neural model: " << options.model << '\n'; return 1; }
        const std::string name =
            "neural_d" + std::to_string(parameters.minimumDepth) +
            "_m" + std::to_string(parameters.minimumLegalMoves) +
            "_w" + std::to_string(parameters.blendPercent);
        conditions.push_back({ name, parameters.minimumDepth, parameters.minimumLegalMoves, parameters.blendPercent, std::move(ai) });
    }
    std::ofstream output(options.output, std::ios::trunc);
    if (!output) { std::cerr << "ERROR: Could not open output: " << options.output << '\n'; return 1; }
    output << "position_id,source_file,source_row,black_bits,white_bits,turn,empty_count,legal_moves,repeat,condition,neural_minimum_depth,neural_minimum_legal_moves,neural_blend_percent,neural_calls,neural_cache_hits,elapsed_ms,nodes,nodes_per_second,target_depth,completed_depth,timed_out,opening_book,tt_hits,move_row,move_col,score,same_move_as_baseline,elapsed_ratio,node_ratio\n";
    output << std::fixed << std::setprecision(3);
    std::vector<Aggregate> aggregates(conditions.size());

    const std::size_t totalRuns = positions.size() * static_cast<std::size_t>(options.repeats) * conditions.size();
    std::size_t completedRuns = 0;
    for (std::size_t positionIndex = 0; positionIndex < positions.size(); ++positionIndex) {
        const auto& position = positions[positionIndex];
        const int emptyCount = 64 - position.board.count(Disc::Black) - position.board.count(Disc::White);
        const int legalMoves = std::popcount(position.board.legalMoves(position.turn));
        for (int repeat = 1; repeat <= options.repeats; ++repeat) {
            std::vector<Result> results(conditions.size());
            std::vector<std::size_t> order(conditions.size());
            for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
            const std::size_t rotation =
                static_cast<std::size_t>((repeat - 1) / 2) % order.size();
            std::rotate(
                order.begin(),
                order.begin() + static_cast<std::ptrdiff_t>(rotation),
                order.end()
            );
            if (repeat % 2 == 0) std::reverse(order.begin(), order.end());
            for (const std::size_t conditionIndex : order) {
                results[conditionIndex] = runSearch(*conditions[conditionIndex].ai, position, options.timeLimitMs);
                std::cout << '\r' << ++completedRuns << '/' << totalRuns << " searches" << std::flush;
            }
            const Result& reference = results.front();
            for (std::size_t i = 0; i < conditions.size(); ++i) {
                const Result& result = results[i];
                const bool sameMove = result.row == reference.row && result.col == reference.col;
                const double nps = result.elapsedMs > 0.0 ? static_cast<double>(result.nodes) * 1000.0 / result.elapsedMs : 0.0;
                const double elapsedRatio = reference.elapsedMs > 0.0 ? result.elapsedMs / reference.elapsedMs : 0.0;
                const double nodeRatio = reference.nodes > 0 ? static_cast<double>(result.nodes) / reference.nodes : 0.0;
                Aggregate& aggregate = aggregates[i];
                ++aggregate.runs;
                if (result.completedDepth < reference.completedDepth) {
                    ++aggregate.shallowerRuns;
                } else if (result.completedDepth > reference.completedDepth) {
                    ++aggregate.deeperRuns;
                }
                if (result.timedOut) ++aggregate.timeoutRuns;
                if (sameMove) ++aggregate.sameMoveRuns;
                aggregate.neuralOrderingCalls += result.neuralOrderingCalls;
                aggregate.neuralOrderingCacheHits +=
                    result.neuralOrderingCacheHits;
                if (
                    !reference.timedOut &&
                    !result.timedOut &&
                    result.completedDepth == reference.completedDepth &&
                    reference.elapsedMs > 0.0 &&
                    reference.nodes > 0
                ) {
                    ++aggregate.comparableRuns;
                    aggregate.comparableElapsedRatios.push_back(elapsedRatio);
                    aggregate.comparableNodeRatios.push_back(nodeRatio);
                }
                output << positionIndex + 1 << ',' << csvText(position.sourcePath) << ',' << position.sourceRow << ',' << hexBits(position.board.black()) << ',' << hexBits(position.board.white()) << ',' << (position.turn == Disc::Black ? "black" : "white") << ',' << emptyCount << ',' << legalMoves << ',' << repeat << ',' << conditions[i].name << ',' << conditions[i].minimumDepth << ',' << conditions[i].minimumLegalMoves << ',' << conditions[i].blendPercent << ',' << result.neuralOrderingCalls << ',' << result.neuralOrderingCacheHits << ',' << result.elapsedMs << ',' << result.nodes << ',' << nps << ',' << result.targetDepth << ',' << result.completedDepth << ',' << result.timedOut << ',' << result.openingBook << ',' << result.transpositionHits << ',' << result.row << ',' << result.col << ',' << result.score << ',' << sameMove << ',' << elapsedRatio << ',' << nodeRatio << '\n';
            }
        }
    }
    if (!output.good()) return 1;
    output.close();

    std::size_t bestIndex = 0;
    for (std::size_t i = 1; i < conditions.size(); ++i) {
        const Aggregate& candidate = aggregates[i];
        const Aggregate& best = aggregates[bestIndex];
        if (candidate.shallowerRuns != best.shallowerRuns) {
            if (candidate.shallowerRuns < best.shallowerRuns) bestIndex = i;
            continue;
        }
        if (candidate.deeperRuns != best.deeperRuns) {
            if (candidate.deeperRuns > best.deeperRuns) bestIndex = i;
            continue;
        }
        const double candidateElapsed =
            geometricMean(candidate.comparableElapsedRatios);
        const double bestElapsed = geometricMean(best.comparableElapsedRatios);
        if (
            candidateElapsed > 0.0 &&
            (bestElapsed == 0.0 || candidateElapsed < bestElapsed)
        ) {
            bestIndex = i;
            continue;
        }
        if (candidateElapsed == 0.0 && bestElapsed == 0.0) {
            const double candidateCalls = candidate.runs > 0
                ? static_cast<double>(candidate.neuralOrderingCalls) /
                    static_cast<double>(candidate.runs)
                : 0.0;
            const double bestCalls = best.runs > 0
                ? static_cast<double>(best.neuralOrderingCalls) /
                    static_cast<double>(best.runs)
                : 0.0;
            if (candidateCalls < bestCalls) bestIndex = i;
        }
    }
    const Aggregate& best = aggregates[bestIndex];
    const double bestElapsed = geometricMean(best.comparableElapsedRatios);
    const double bestNodes = geometricMean(best.comparableNodeRatios);
    const bool depthImproved =
        best.deeperRuns >= 2 &&
        best.deeperRuns * 10 >= best.runs;
    const bool speedImproved =
        best.comparableRuns >= 5 &&
        bestElapsed > 0.0 &&
        bestElapsed <= 0.98 &&
        bestNodes < 1.0;
    const bool recommendNeural =
        bestIndex != 0 &&
        best.shallowerRuns == 0 &&
        (depthImproved || speedImproved);
    const std::size_t recommendedIndex = recommendNeural ? bestIndex : 0;

    std::ofstream summary(options.summary, std::ios::trunc);
    if (!summary) {
        std::cerr << "\nERROR: Could not open summary: " << options.summary << '\n';
        return 1;
    }
    summary << "condition,neural_minimum_depth,neural_minimum_legal_moves,neural_blend_percent,runs,comparable_runs,shallower_runs,deeper_runs,timeouts,same_move_percent,median_elapsed_ratio,geometric_mean_elapsed_ratio,median_node_ratio,geometric_mean_node_ratio,mean_neural_calls,mean_neural_cache_hits,recommended\n";
    summary << std::fixed << std::setprecision(3);
    for (std::size_t i = 0; i < conditions.size(); ++i) {
        const Aggregate& aggregate = aggregates[i];
        const double sameMovePercent = aggregate.runs > 0
            ? static_cast<double>(aggregate.sameMoveRuns) * 100.0 /
                static_cast<double>(aggregate.runs)
            : 0.0;
        const double meanCalls = aggregate.runs > 0
            ? static_cast<double>(aggregate.neuralOrderingCalls) /
                static_cast<double>(aggregate.runs)
            : 0.0;
        const double meanCacheHits = aggregate.runs > 0
            ? static_cast<double>(aggregate.neuralOrderingCacheHits) /
                static_cast<double>(aggregate.runs)
            : 0.0;
        summary << conditions[i].name << ','
                << conditions[i].minimumDepth << ','
                << conditions[i].minimumLegalMoves << ','
                << conditions[i].blendPercent << ','
                << aggregate.runs << ','
                << aggregate.comparableRuns << ','
                << aggregate.shallowerRuns << ','
                << aggregate.deeperRuns << ','
                << aggregate.timeoutRuns << ','
                << sameMovePercent << ','
                << median(aggregate.comparableElapsedRatios) << ','
                << geometricMean(aggregate.comparableElapsedRatios) << ','
                << median(aggregate.comparableNodeRatios) << ','
                << geometricMean(aggregate.comparableNodeRatios) << ','
                << meanCalls << ','
                << meanCacheHits << ','
                << (i == recommendedIndex ? 1 : 0) << '\n';
    }
    if (!summary.good()) return 1;

    std::cout << "\nWrote " << totalRuns << " rows to " << options.output
              << "\nWrote aggregate results to " << options.summary << '\n';
    if (recommendNeural) {
        const Condition& recommendation = conditions[recommendedIndex];
        std::cout
            << "Recommended game.ini values:\n"
            << "neural_ordering_enabled=true\n"
            << "neural_ordering_minimum_depth="
            << recommendation.minimumDepth << '\n'
            << "neural_ordering_minimum_legal_moves="
            << recommendation.minimumLegalMoves << '\n'
            << "neural_ordering_blend_percent="
            << recommendation.blendPercent << '\n';
    } else {
        std::cout
            << "Recommendation: neural_ordering_enabled=false\n"
            << "No neural condition passed the conservative speed/depth rule.\n";
    }
    return 0;
}
