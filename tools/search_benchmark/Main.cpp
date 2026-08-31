#include "ai/OthelloAI.h"
#include "ai/benchmark/SearchPositionCsv.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
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
        std::string model = "data/model/othello_ordering.model";
        std::vector<int> minimumDepths;
        int depth = 12;
        int exactEndgameEmpty = 0;
        int timeLimitMs = 10'000;
        int repeats = 1;
        int minimumEmpty = 17;
        int maximumEmpty = 43;
        std::size_t maximumPositions = 10;
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
        std::unique_ptr<OthelloAI> ai;
    };
    struct Result {
        double elapsedMs = 0.0;
        std::uint64_t nodes = 0;
        std::uint64_t transpositionHits = 0;
        int targetDepth = 0;
        int completedDepth = 0;
        bool timedOut = false;
        bool openingBook = false;
        int row = -1;
        int col = -1;
        int score = 0;
    };

    void printUsage() {
        std::cout <<
            "Fixed-position neural move-ordering benchmark\n\n"
            "Required:\n  --input PATH              Search-statistics CSV (repeatable)\n\n"
            "Options:\n"
            "  --output PATH             Output CSV (default: neural_ordering_benchmark.csv)\n"
            "  --model PATH              Neural model path\n"
            "  --depth N                 Search depth 1-12 (default: 12)\n"
            "  --minimum-depth N         Neural minimum depth (repeatable; default: 10,11,12)\n"
            "  --exact-endgame-empty N   Exact-search threshold 0-20 (default: 0)\n"
            "  --time-limit-ms N         Deadline per search (default: 10000)\n"
            "  --repeats N               Repeats per position (default: 1)\n"
            "  --min-empty N             Minimum empty squares (default: 17)\n"
            "  --max-empty N             Maximum empty squares (default: 43)\n"
            "  --max-positions N         Position limit; 0 means unlimited (default: 10)\n"
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
            if (index + 1 >= argc) { error = "Missing value after " + argument; return false; }
            const std::string value = argv[++index];
            int number = 0;
            if (argument == "--input") options.inputs.push_back(value);
            else if (argument == "--output") options.output = value;
            else if (argument == "--model") options.model = value;
            else if (!parseInteger(value, number)) { error = "Invalid integer for " + argument + ": " + value; return false; }
            else if (argument == "--depth") options.depth = number;
            else if (argument == "--minimum-depth") options.minimumDepths.push_back(number);
            else if (argument == "--exact-endgame-empty") options.exactEndgameEmpty = number;
            else if (argument == "--time-limit-ms") options.timeLimitMs = number;
            else if (argument == "--repeats") options.repeats = number;
            else if (argument == "--min-empty") options.minimumEmpty = number;
            else if (argument == "--max-empty") options.maximumEmpty = number;
            else if (argument == "--max-positions") {
                if (number < 0) { error = "--max-positions must be at least 0"; return false; }
                options.maximumPositions = static_cast<std::size_t>(number);
            } else { error = "Unknown option: " + argument; return false; }
        }
        if (options.inputs.empty()) error = "At least one --input is required";
        else if (options.depth < 1 || options.depth > 12) error = "--depth must be between 1 and 12";
        else if (options.exactEndgameEmpty < 0 || options.exactEndgameEmpty > 20) error = "--exact-endgame-empty must be between 0 and 20";
        else if (options.timeLimitMs < 1 || options.timeLimitMs > 60'000) error = "--time-limit-ms must be between 1 and 60000";
        else if (options.repeats < 1 || options.repeats > 100) error = "--repeats must be between 1 and 100";
        else if (options.minimumEmpty < 0 || options.maximumEmpty > 60 || options.minimumEmpty > options.maximumEmpty) error = "Empty-square range must be within 0-60";
        if (!error.empty()) return false;
        if (options.minimumDepths.empty()) options.minimumDepths = { 10, 11, 12 };
        std::sort(options.minimumDepths.begin(), options.minimumDepths.end());
        options.minimumDepths.erase(std::unique(options.minimumDepths.begin(), options.minimumDepths.end()), options.minimumDepths.end());
        for (const int depth : options.minimumDepths) {
            if (depth < 1 || depth > 64) { error = "--minimum-depth must be between 1 and 64"; return false; }
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
        result.targetDepth = progress.targetDepth.load(std::memory_order_relaxed);
        result.completedDepth = progress.completedDepth.load(std::memory_order_relaxed);
        result.timedOut = progress.timedOut.load(std::memory_order_relaxed);
        if (move) {
            result.openingBook = move->openingBook;
            result.row = move->row; result.col = move->col; result.score = move->score;
            result.nodes = move->searchedNodes;
            result.transpositionHits = move->transpositionHits;
            result.completedDepth = move->completedIterations;
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
    conditions.push_back({ "baseline", 0, std::move(baseline) });
    for (const int minimumDepth : options.minimumDepths) {
        auto ai = std::make_unique<OthelloAI>(options.depth);
        ai->setExactEndgameEmpty(options.exactEndgameEmpty);
        if (!ai->configureNeuralOrdering(true, options.model, minimumDepth)) { std::cerr << "ERROR: Could not load neural model: " << options.model << '\n'; return 1; }
        conditions.push_back({ "neural_min_" + std::to_string(minimumDepth), minimumDepth, std::move(ai) });
    }
    std::ofstream output(options.output, std::ios::trunc);
    if (!output) { std::cerr << "ERROR: Could not open output: " << options.output << '\n'; return 1; }
    output << "position_id,source_file,source_row,black_bits,white_bits,turn,empty_count,legal_moves,repeat,condition,neural_minimum_depth,elapsed_ms,nodes,nodes_per_second,target_depth,completed_depth,timed_out,opening_book,tt_hits,move_row,move_col,score,same_move_as_baseline,elapsed_ratio,node_ratio\n";
    output << std::fixed << std::setprecision(3);

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
                output << positionIndex + 1 << ',' << csvText(position.sourcePath) << ',' << position.sourceRow << ',' << hexBits(position.board.black()) << ',' << hexBits(position.board.white()) << ',' << (position.turn == Disc::Black ? "black" : "white") << ',' << emptyCount << ',' << legalMoves << ',' << repeat << ',' << conditions[i].name << ',' << conditions[i].minimumDepth << ',' << result.elapsedMs << ',' << result.nodes << ',' << nps << ',' << result.targetDepth << ',' << result.completedDepth << ',' << result.timedOut << ',' << result.openingBook << ',' << result.transpositionHits << ',' << result.row << ',' << result.col << ',' << result.score << ',' << sameMove << ',' << elapsedRatio << ',' << nodeRatio << '\n';
            }
        }
    }
    std::cout << "\nWrote " << totalRuns << " rows to " << options.output << '\n';
    return output.good() ? 0 : 1;
}
