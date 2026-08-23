#include "ai/SearchStatisticsLogger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
    constexpr const char* Header =
        "timestamp,difficulty,turn,black_bits,white_bits,empty_count,"
        "legal_moves,configured_depth,target_depth,completed_depth,"
        "exact_search,timed_out,time_limit_ms,elapsed_ms,nodes,"
        "nodes_per_second,tt_hits,tt_hit_percent,neural_enabled,"
        "neural_active,neural_minimum_depth,move_row,move_col,score,"
        "opening_book\n";

    std::string timestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &time);
#else
        localtime_r(&time, &local);
#endif
        std::ostringstream output;
        output << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
        return output.str();
    }

    std::string escaped(const std::string& value) {
        if (value.find_first_of(",\"\r\n") == std::string::npos) {
            return value;
        }

        std::string result = "\"";
        for (const char character : value) {
            if (character == '"') result += '"';
            result += character;
        }
        result += '"';
        return result;
    }

    const char* discName(Disc disc) noexcept {
        if (disc == Disc::Black) return "black";
        if (disc == Disc::White) return "white";
        return "empty";
    }
}

bool SearchStatisticsLogger::Append(
    const std::string& path,
    const SearchStatisticsEntry& entry
) noexcept {
    try {
        const std::filesystem::path outputPath(path);
        const auto parent = outputPath.parent_path();
        if (!parent.empty()) {
            std::error_code error;
            std::filesystem::create_directories(parent, error);
            if (error) return false;
        }

        std::error_code error;
        const bool writeHeader =
            !std::filesystem::exists(outputPath, error) ||
            (!error && std::filesystem::file_size(outputPath, error) == 0);
        if (error) return false;

        std::ofstream output(outputPath, std::ios::app);
        if (!output) return false;
        if (writeHeader) output << Header;

        const double nodesPerSecond = entry.elapsedMs > 0
            ? static_cast<double>(entry.searchedNodes) * 1000.0 /
                static_cast<double>(entry.elapsedMs)
            : 0.0;
        const double transpositionHitPercent = entry.searchedNodes > 0
            ? static_cast<double>(entry.transpositionHits) * 100.0 /
                static_cast<double>(entry.searchedNodes)
            : 0.0;

        output
            << timestamp() << ','
            << escaped(entry.difficultyName) << ','
            << discName(entry.turn) << ','
            << "0x" << std::hex << std::setw(16) << std::setfill('0')
            << entry.black << ','
            << "0x" << std::setw(16) << entry.white
            << std::dec << std::setfill(' ') << ','
            << entry.emptyCount << ','
            << entry.legalMoveCount << ','
            << entry.configuredDepth << ','
            << entry.targetDepth << ','
            << entry.completedDepth << ','
            << (entry.exactSearch ? 1 : 0) << ','
            << (entry.timedOut ? 1 : 0) << ','
            << entry.timeLimitMs << ','
            << entry.elapsedMs << ','
            << entry.searchedNodes << ','
            << std::fixed << std::setprecision(2) << nodesPerSecond << ','
            << entry.transpositionHits << ','
            << transpositionHitPercent << ','
            << (entry.neuralOrderingEnabled ? 1 : 0) << ','
            << (entry.neuralOrderingActive ? 1 : 0) << ','
            << entry.neuralOrderingMinimumDepth << ','
            << entry.moveRow << ','
            << entry.moveCol << ','
            << entry.score << ','
            << (entry.openingBook ? 1 : 0)
            << '\n';
        return output.good();
    } catch (...) {
        return false;
    }
}
