#include "ai/benchmark/SearchPositionCsv.h"
#include <algorithm>
#include <charconv>
#include <fstream>
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
