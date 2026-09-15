#include "ai/benchmark/SearchPositionCsv.h"
#include <algorithm>
#include <bit>
#include <charconv>
#include <fstream>
#include <random>
#include <string_view>
#include <unordered_map>

namespace {
    bool parseCsvLine(const std::string& line, std::vector<std::string>& fields) {
        fields.clear();
        std::string field;
        bool quoted = false;
        for (std::size_t i = 0; i < line.size(); ++i) {
            const char character = line[i];
            if (quoted) {
                if (character == '"') {
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        field += '"';
                        ++i;
                    } else quoted = false;
                } else field += character;
            } else if (character == ',') {
                fields.push_back(std::move(field));
                field.clear();
            } else if (character == '"' && field.empty()) quoted = true;
            else field += character;
        }
        if (quoted) return false;
        if (!field.empty() && field.back() == '\r') field.pop_back();
        fields.push_back(std::move(field));
        return true;
    }

    bool parseBits(std::string_view text, BitBoard::Bits& value) {
        if (text.starts_with("0x") || text.starts_with("0X")) text.remove_prefix(2);
        if (text.empty()) return false;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 16);
        return result.ec == std::errc{} && result.ptr == text.data() + text.size();
    }

    Disc parseDisc(std::string_view text) noexcept {
        if (text == "black") return Disc::Black;
        if (text == "white") return Disc::White;
        return Disc::Empty;
    }

    Disc opponentOf(Disc disc) noexcept {
        return disc == Disc::Black ? Disc::White : Disc::Black;
    }

    int randomMoveIndex(BitBoard::Bits moves, std::mt19937& random) {
        std::uniform_int_distribution<int> distribution(
            0,
            std::popcount(moves) - 1
        );
        int selected = distribution(random);
        while (selected-- > 0) moves &= moves - 1;
        return std::countr_zero(moves);
    }

    int generatedMoveIndex(
        const BitBoard& board,
        Disc turn,
        BitBoard::Bits legalMoves,
        std::mt19937& random
    ) {
        std::uniform_int_distribution<int> percent(0, 99);
        if (percent(random) < 30) {
            return randomMoveIndex(legalMoves, random);
        }

        int bestIndex = -1;
        int bestScore = -1'000'000;
        BitBoard::Bits candidates = legalMoves;
        while (candidates != 0) {
            const int index = std::countr_zero(candidates);
            BitBoard child = board;
            child.put(turn, index / BitBoard::Size, index % BitBoard::Size);
            const bool corner =
                index == 0 || index == 7 || index == 56 || index == 63;
            const int score =
                (corner ? 10'000 : 0) -
                std::popcount(child.legalMoves(opponentOf(turn))) * 100;
            if (score > bestScore) {
                bestScore = score;
                bestIndex = index;
            }
            candidates &= candidates - 1;
        }
        return bestIndex;
    }
}

bool SearchBenchmark::LoadPositions(const std::string& path, std::vector<Position>& positions, std::string& error) {
    std::ifstream input(path);
    if (!input) {
        error = "Could not open input: " + path;
        return false;
    }
    std::string line;
    std::vector<std::string> fields;
    if (!std::getline(input, line) || !parseCsvLine(line, fields)) {
        error = "Could not read CSV header: " + path;
        return false;
    }
    if (!fields.empty() && fields.front().starts_with("\xEF\xBB\xBF")) fields.front().erase(0, 3);

    std::unordered_map<std::string, std::size_t> columns;
    for (std::size_t i = 0; i < fields.size(); ++i) columns.emplace(fields[i], i);
    constexpr const char* Required[] = { "black_bits", "white_bits", "turn", "opening_book" };
    for (const char* name : Required) {
        if (!columns.contains(name)) {
            error = "Missing column '" + std::string(name) + "' in " + path;
            return false;
        }
    }
    const auto blackColumn = columns.at("black_bits");
    const auto whiteColumn = columns.at("white_bits");
    const auto turnColumn = columns.at("turn");
    const auto openingBookColumn = columns.at("opening_book");
    const std::size_t maximumColumn = std::max(std::max(blackColumn, whiteColumn), std::max(turnColumn, openingBookColumn));

    std::size_t row = 1;
    while (std::getline(input, line)) {
        ++row;
        if (line.empty()) continue;
        if (!parseCsvLine(line, fields) || fields.size() <= maximumColumn) {
            error = "Malformed CSV row " + std::to_string(row) + " in " + path;
            return false;
        }
        if (fields[openingBookColumn] == "1") continue;
        BitBoard::Bits black = 0;
        BitBoard::Bits white = 0;
        const Disc turn = parseDisc(fields[turnColumn]);
        if (!parseBits(fields[blackColumn], black) || !parseBits(fields[whiteColumn], white) || turn == Disc::Empty) {
            error = "Invalid position at row " + std::to_string(row) + " in " + path;
            return false;
        }
        auto board = BitBoard::FromBits(black, white);
        if (!board || !board->hasAnyMove(turn)) {
            error = "Illegal position at row " + std::to_string(row) + " in " + path;
            return false;
        }
        positions.push_back({ *board, turn, path, row });
    }
    return true;
}

bool SearchBenchmark::GeneratePositions(
    const GenerationOptions& options,
    std::vector<Position>& positions,
    std::string& error
) {
    if (options.count == 0) return true;
    if (
        options.count > 10'000 ||
        options.minimumEmpty < 0 ||
        options.maximumEmpty > 60 ||
        options.minimumEmpty > options.maximumEmpty
    ) {
        error = "Generated-position count or empty-square range is invalid";
        return false;
    }

    std::mt19937 random(options.seed);
    std::uniform_int_distribution<int> targetEmpty(
        options.minimumEmpty,
        options.maximumEmpty
    );
    const std::size_t targetSize = positions.size() + options.count;
    const std::size_t maximumGames = options.count * 100 + 1'000;
    std::size_t game = 0;
    while (positions.size() < targetSize && game < maximumGames) {
        ++game;
        BitBoard board;
        Disc turn = Disc::Black;
        const int target = targetEmpty(random);
        while (!board.isGameOver()) {
            const BitBoard::Bits legalMoves = board.legalMoves(turn);
            if (legalMoves == 0) {
                turn = opponentOf(turn);
                continue;
            }
            const int emptyCount =
                64 - board.count(Disc::Black) - board.count(Disc::White);
            if (emptyCount <= target) {
                positions.push_back({
                    board,
                    turn,
                    "generated(seed=" + std::to_string(options.seed) + ")",
                    game,
                });
                break;
            }
            const int moveIndex = generatedMoveIndex(
                board,
                turn,
                legalMoves,
                random
            );
            board.put(
                turn,
                moveIndex / BitBoard::Size,
                moveIndex % BitBoard::Size
            );
            turn = opponentOf(turn);
        }
    }
    if (positions.size() != targetSize) {
        error = "Could not generate enough legal positions for the "
            "requested empty-square range";
        return false;
    }
    return true;
}
