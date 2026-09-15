#include "ai/OthelloAI.h"
#include "ai/training/OthelloTrainingData.h"
#include "model/BitBoard.h"

#include <bit>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {
struct Options {
    std::string outputPath = "othello_training.csv";
    std::size_t positionCount = 1'000;
    int teacherDepth = 5;
    int exactEndgameEmpty = 12;
    int explorationPercent = 30;
    int minimumEmpty = 12;
    int maximumEmpty = 52;
    int minimumLegalMoves = 2;
    std::uint32_t seed = 0x5eed1234U;
    bool augmentSymmetries = true;
};

struct PositionKey {
    BitBoard::Bits black = 0;
    BitBoard::Bits white = 0;
    Disc turn = Disc::Empty;

    bool operator==(const PositionKey&) const noexcept = default;
};

struct PositionKeyHash {
    std::size_t operator()(const PositionKey& key) const noexcept {
        const auto turn = static_cast<std::uint64_t>(key.turn);
        return static_cast<std::size_t>(
            key.black ^ (key.white * 0x9e3779b97f4a7c15ULL) ^
            (turn << 61)
        );
    }
};

bool parseInteger(std::string_view text, std::uint64_t& value) {
    const char* first = text.data();
    const char* last = first + text.size();
    const auto result = std::from_chars(first, last, value);
    return result.ec == std::errc{} && result.ptr == last;
}

bool parseOptions(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--no-augmentation") {
            options.augmentSymmetries = false;
            continue;
        }
        if (i + 1 >= argc) return false;

        const std::string_view value = argv[++i];
        if (argument == "--output") {
            options.outputPath = value;
            continue;
        }

        std::uint64_t number = 0;
        if (!parseInteger(value, number)) return false;
        if (argument == "--positions") {
            options.positionCount = static_cast<std::size_t>(number);
        } else if (argument == "--depth") {
            options.teacherDepth = static_cast<int>(number);
        } else if (argument == "--exact-endgame-empty") {
            options.exactEndgameEmpty = static_cast<int>(number);
        } else if (argument == "--exploration-percent") {
            options.explorationPercent = static_cast<int>(number);
        } else if (argument == "--min-empty") {
            options.minimumEmpty = static_cast<int>(number);
        } else if (argument == "--max-empty") {
            options.maximumEmpty = static_cast<int>(number);
        } else if (argument == "--minimum-legal-moves") {
            options.minimumLegalMoves = static_cast<int>(number);
        } else if (argument == "--seed") {
            options.seed = static_cast<std::uint32_t>(number);
        } else {
            return false;
        }
    }

    return
        options.positionCount > 0 &&
        1 <= options.teacherDepth && options.teacherDepth <= 12 &&
        0 <= options.exactEndgameEmpty && options.exactEndgameEmpty <= 20 &&
        0 <= options.explorationPercent &&
        options.explorationPercent <= 100 &&
        0 <= options.minimumEmpty &&
        options.minimumEmpty <= options.maximumEmpty &&
        options.maximumEmpty <= 60 &&
        1 <= options.minimumLegalMoves &&
        options.minimumLegalMoves <= 64;
}

void printUsage() {
    std::cout
        << "Usage: UchinokoOthelloTrainingData [options]\n"
        << "  --output PATH                 Output CSV path\n"
        << "  --positions N                Unique source positions (default: 1000)\n"
        << "  --depth N                    Teacher search depth (default: 5)\n"
        << "  --exact-endgame-empty N      Exact-search threshold (default: 12)\n"
        << "  --exploration-percent N      Random self-play moves (default: 30)\n"
        << "  --min-empty N                Smallest recorded empty count (default: 12)\n"
        << "  --max-empty N                Largest recorded empty count (default: 52)\n"
        << "  --minimum-legal-moves N      Recorded branching threshold (default: 2)\n"
        << "  --seed N                     Reproducible random seed\n"
        << "  --no-augmentation            Do not emit eight board symmetries\n";
    std::cout
        << "\nEach row contains 128 board features followed by one search "
        << "score per square. Illegal moves are written as x.\n";
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
}

int main(int argc, char** argv) {
    Options options;
    if (!parseOptions(argc, argv, options)) {
        printUsage();
        return 1;
    }

    OthelloTrainingData::CsvWriter writer;
    if (!writer.OpenPolicyScores(options.outputPath)) {
        std::cerr << "Could not open output: " << options.outputPath << '\n';
        return 1;
    }

    OthelloAI teacher(options.teacherDepth);
    teacher.setExactEndgameEmpty(options.exactEndgameEmpty);
    std::mt19937 random(options.seed);
    std::uniform_int_distribution<int> percent(0, 99);
    std::unordered_set<PositionKey, PositionKeyHash> seen;
    seen.reserve(options.positionCount * 2);

    std::size_t generatedPositions = 0;
    std::size_t games = 0;
    while (generatedPositions < options.positionCount) {
        BitBoard board;
        Disc turn = Disc::Black;
        ++games;

        while (
            !board.isGameOver() &&
            generatedPositions < options.positionCount
        ) {
            const BitBoard::Bits legalMoves = board.legalMoves(turn);
            if (legalMoves == 0) {
                turn = opponentOf(turn);
                continue;
            }

            const PositionKey key{ board.black(), board.white(), turn };
            const int emptyCount =
                64 - board.count(Disc::Black) - board.count(Disc::White);
            const bool recordPosition =
                options.minimumEmpty <= emptyCount &&
                emptyCount <= options.maximumEmpty &&
                std::popcount(legalMoves) >= options.minimumLegalMoves &&
                !seen.contains(key);

            int teacherIndex = -1;
            if (recordPosition) {
                const auto teacherMoves = teacher.analyzeLegalMoves(
                    board,
                    turn,
                    options.teacherDepth
                );
                if (teacherMoves.empty()) {
                    std::cerr
                        << "Teacher returned no scores for a legal position.\n";
                    return 1;
                }
                OthelloTrainingData::MoveScores teacherScores;
                teacherScores.fill(OthelloTrainingData::IllegalMoveScore);
                const OthelloAI::Move* bestTeacherMove =
                    &teacherMoves.front();
                for (const auto& move : teacherMoves) {
                    const int index =
                        move.row * BitBoard::Size + move.col;
                    teacherScores[static_cast<std::size_t>(index)] =
                        move.score;
                    if (move.score > bestTeacherMove->score) {
                        bestTeacherMove = &move;
                    }
                }
                teacherIndex =
                    bestTeacherMove->row * BitBoard::Size +
                    bestTeacherMove->col;
                if (!writer.WritePolicyScores(
                    board,
                    turn,
                    teacherScores,
                    options.augmentSymmetries
                )) {
                    std::cerr << "Could not write training row.\n";
                    return 1;
                }
                seen.insert(key);
                ++generatedPositions;
                if (
                    generatedPositions % 100 == 0 ||
                    generatedPositions == options.positionCount
                ) {
                    std::cout
                        << "Positions: " << generatedPositions << '/'
                        << options.positionCount
                        << ", rows: " << writer.rowsWritten()
                        << ", games: " << games << '\n';
                }
            }

            if (teacherIndex < 0) {
                const auto teacherMove = teacher.chooseMove(board, turn);
                if (!teacherMove) {
                    std::cerr
                        << "Teacher returned no move for a legal position.\n";
                    return 1;
                }
                teacherIndex =
                    teacherMove->row * BitBoard::Size + teacherMove->col;
            }

            int playedIndex = teacherIndex;
            if (percent(random) < options.explorationPercent) {
                playedIndex = randomMoveIndex(legalMoves, random);
            }
            if (!board.put(
                turn,
                playedIndex / BitBoard::Size,
                playedIndex % BitBoard::Size
            )) {
                std::cerr << "Could not apply selected self-play move.\n";
                return 1;
            }
            turn = opponentOf(turn);
        }
    }

    std::cout
        << "Wrote " << writer.rowsWritten() << " rows to "
        << options.outputPath
        << " (policy-score format v2)\n";
    return 0;
}
