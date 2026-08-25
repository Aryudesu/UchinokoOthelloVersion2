#include "ai/OthelloAI.h"
#include "ai/NeuralMoveOrderer.h"
#include "ai/inference/ModelFormat.h"
#include "ai/training/OthelloTrainingData.h"
#include "ai/OpeningBook.h"
#include "ai/OpeningBookData.h"
#include "ai/SearchStatisticsLogger.h"
#include "model/BitBoard.h"
#include "model/MatchResult.h"

#include <bit>
#include <array>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <iostream>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    using Bits = BitBoard::Bits;
    constexpr const char* NeuralTestModelPath =
        "core_test_ordering.model";

    void require(bool condition, const std::string& message) {
        if (!condition) throw std::runtime_error(message);
    }

    bool inside(int row, int col) {
        return 0 <= row && row < BitBoard::Size &&
               0 <= col && col < BitBoard::Size;
    }

    Disc opposite(Disc disc) {
        return disc == Disc::Black ? Disc::White : Disc::Black;
    }

    Bits slowFlips(
        const BitBoard& board,
        Disc disc,
        int row,
        int col
    ) {
        if (
            disc == Disc::Empty ||
            !inside(row, col) ||
            board.discAt(row, col) != Disc::Empty
        ) {
            return 0;
        }

        constexpr int directions[8][2] = {
            { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 },
            { 0, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
        };

        Bits result = 0;
        const Disc opponent = opposite(disc);
        for (const auto& direction : directions) {
            int currentRow = row + direction[0];
            int currentCol = col + direction[1];
            Bits candidate = 0;

            while (
                inside(currentRow, currentCol) &&
                board.discAt(currentRow, currentCol) == opponent
            ) {
                candidate |= BitBoard::bitAt(currentRow, currentCol);
                currentRow += direction[0];
                currentCol += direction[1];
            }

            if (
                candidate != 0 &&
                inside(currentRow, currentCol) &&
                board.discAt(currentRow, currentCol) == disc
            ) {
                result |= candidate;
            }
        }
        return result;
    }

    Bits slowLegalMoves(const BitBoard& board, Disc disc) {
        Bits result = 0;
        for (int row = 0; row < BitBoard::Size; ++row) {
            for (int col = 0; col < BitBoard::Size; ++col) {
                if (slowFlips(board, disc, row, col) != 0) {
                    result |= BitBoard::bitAt(row, col);
                }
            }
        }
        return result;
    }

    void verifyAgainstReference(const BitBoard& board) {
        require(
            (board.black() & board.white()) == 0,
            "Black and white bitboards overlap"
        );
        require(
            board.count(Disc::Black) == std::popcount(board.black()),
            "Black count differs from bitboard"
        );
        require(
            board.count(Disc::White) == std::popcount(board.white()),
            "White count differs from bitboard"
        );

        for (Disc disc : { Disc::Black, Disc::White }) {
            require(
                board.legalMoves(disc) == slowLegalMoves(board, disc),
                "Optimized legal move generation differs from reference"
            );

            for (int row = 0; row < BitBoard::Size; ++row) {
                for (int col = 0; col < BitBoard::Size; ++col) {
                    require(
                        board.flipsFor(disc, row, col) ==
                            slowFlips(board, disc, row, col),
                        "Optimized flip calculation differs from reference"
                    );
                }
            }
        }
    }

    int selectMoveIndex(Bits moves, std::uint32_t& randomState) {
        randomState = randomState * 1664525u + 1013904223u;
        int selected = static_cast<int>(
            randomState % static_cast<std::uint32_t>(std::popcount(moves))
        );

        while (selected-- > 0) moves &= moves - 1;
        return std::countr_zero(moves);
    }

    void testInitialPosition() {
        BitBoard board;

        require(board.count(Disc::Black) == 2, "Initial black count must be 2");
        require(board.count(Disc::White) == 2, "Initial white count must be 2");
        require(board.discAt(3, 4) == Disc::Black, "Initial black disc missing");
        require(board.discAt(4, 3) == Disc::Black, "Initial black disc missing");
        require(board.discAt(3, 3) == Disc::White, "Initial white disc missing");
        require(board.discAt(4, 4) == Disc::White, "Initial white disc missing");

        const Bits expectedBlackMoves =
            BitBoard::bitAt(2, 3) |
            BitBoard::bitAt(3, 2) |
            BitBoard::bitAt(4, 5) |
            BitBoard::bitAt(5, 4);
        require(
            board.legalMoves(Disc::Black) == expectedBlackMoves,
            "Initial legal moves are incorrect"
        );
        require(!board.put(Disc::Black, 0, 0), "Illegal move was accepted");
        require(!board.put(Disc::Empty, 2, 3), "Empty disc move was accepted");
        require(board.discAt(-1, 0) == Disc::Empty, "Out-of-range read failed");

        verifyAgainstReference(board);
    }

    void testMatchResult() {
        BitBoard board;
        const MatchResult initial = MatchResult::From(board);
        require(initial.blackCount == 2, "Initial result black count is wrong");
        require(initial.whiteCount == 2, "Initial result white count is wrong");
        require(initial.difference == 0, "Initial result difference is wrong");
        require(initial.winner == MatchWinner::Draw, "Equal score must be draw");

        require(board.put(Disc::Black, 4, 5), "Could not build result test");
        const MatchResult blackLead = MatchResult::From(board);
        require(blackLead.blackCount == 4, "Result black count is wrong");
        require(blackLead.whiteCount == 1, "Result white count is wrong");
        require(blackLead.difference == 3, "Result difference is wrong");
        require(
            blackLead.winner == MatchWinner::Black,
            "Black lead was not recognized"
        );
    }

    void testRandomGamesAndPasses() {
        std::uint32_t randomState = 0x12345678u;
        bool observedPass = false;

        for (int game = 0; game < 128; ++game) {
            BitBoard board;
            Disc turn = Disc::Black;
            int consecutivePasses = 0;

            while (consecutivePasses < 2) {
                verifyAgainstReference(board);
                Bits moves = board.legalMoves(turn);

                if (moves == 0) {
                    if (board.hasAnyMove(opposite(turn))) {
                        observedPass = true;
                    }
                    ++consecutivePasses;
                    turn = opposite(turn);
                    continue;
                }

                consecutivePasses = 0;
                const int index = selectMoveIndex(moves, randomState);
                const int row = index / BitBoard::Size;
                const int col = index % BitBoard::Size;
                const int beforeCount =
                    board.count(Disc::Black) + board.count(Disc::White);

                require(board.put(turn, row, col), "Legal move was rejected");
                require(
                    board.count(Disc::Black) + board.count(Disc::White) ==
                        beforeCount + 1,
                    "A move must add exactly one occupied square"
                );
                turn = opposite(turn);
            }

            verifyAgainstReference(board);
            require(board.isGameOver(), "Completed game is not game over");
        }

        require(observedPass, "No pass position was exercised");
    }

    int finalDifference(const BitBoard& board, Disc perspective) {
        return board.count(perspective) -
               board.count(opposite(perspective));
    }

    int solveExactly(
        const BitBoard& board,
        Disc turn,
        Disc perspective
    ) {
        Bits moves = board.legalMoves(turn);
        const Disc opponent = opposite(turn);

        if (moves == 0) {
            if (!board.hasAnyMove(opponent)) {
                return finalDifference(board, perspective);
            }
            return solveExactly(board, opponent, perspective);
        }

        int best = turn == perspective
            ? std::numeric_limits<int>::min()
            : std::numeric_limits<int>::max();

        while (moves != 0) {
            const int index = std::countr_zero(moves);
            moves &= moves - 1;

            BitBoard child = board;
            require(
                child.put(
                    turn,
                    index / BitBoard::Size,
                    index % BitBoard::Size
                ),
                "Reference solver failed to apply move"
            );
            const int score = solveExactly(child, opponent, perspective);

            if (turn == perspective) {
                if (score > best) best = score;
            } else {
                if (score < best) best = score;
            }
        }
        return best;
    }

    void testLegacyOpeningBook() {
        require(
            LegacyOpeningLines.size() == 244,
            "Legacy opening line count changed"
        );

        for (const OpeningLine& line : LegacyOpeningLines) {
            BitBoard board;
            Disc turn = Disc::Black;
            require(
                !line.moves.empty() && line.moves.size() % 2 == 0,
                "Opening notation has an invalid length"
            );

            for (std::size_t i = 0; i < line.moves.size(); i += 2) {
                const int col = line.moves[i] - 'a';
                const int row = line.moves[i + 1] - '1';
                require(
                    board.put(turn, row, col),
                    "Legacy opening contains an illegal move"
                );
                turn = opposite(turn);
            }
        }

        OpeningBook book;
        require(
            book.lineCount() == LegacyOpeningLines.size(),
            "OpeningBook rejected a legacy opening line"
        );
        require(
            book.positionCount() > 1'000,
            "OpeningBook contains too few normalized positions"
        );

        BitBoard rabbit;
        Disc rabbitTurn = Disc::Black;
        const auto playUntil = [&](std::string_view notation) {
            const std::size_t occupiedMoves =
                static_cast<std::size_t>(
                    rabbit.count(Disc::Black) + rabbit.count(Disc::White) - 4
                );
            for (std::size_t i = occupiedMoves * 2; i < notation.size(); i += 2) {
                const int col = notation[i] - 'a';
                const int row = notation[i + 1] - '1';
                require(
                    rabbit.put(rabbitTurn, row, col),
                    "Could not replay named opening"
                );
                rabbitTurn = opposite(rabbitTurn);
            }
        };

        playUntil(LegacyOpeningLines[0].moves);
        require(
            book.completedName(rabbit, rabbitTurn) ==
                LegacyOpeningLines[0].name,
            "Short opening name was not recognized"
        );

        playUntil(LegacyOpeningLines[1].moves);
        require(
            book.completedName(rabbit, rabbitTurn) ==
                LegacyOpeningLines[1].name,
            "Opening name did not advance to the deeper variation"
        );
    }

    void testAiReturnsLegalMove() {
        BitBoard board;
        OthelloAI ai(3);

        const auto move = ai.chooseMove(board, Disc::Black);
        require(move.has_value(), "AI returned no move in initial position");
        require(
            board.canPut(Disc::Black, move->row, move->col),
            "AI returned an illegal move"
        );
        require(move->openingBook, "Initial move did not use opening book");
        require(move->searchedNodes == 0, "Opening book unexpectedly searched");
        require(move->searchDepth == 0, "Opening book has a search depth");

        // Leave the stored line and verify that the normal search still works.
        const int line[] = {
            4 * 8 + 5, 5 * 8 + 5, 5 * 8 + 4, 3 * 8 + 5,
            2 * 8 + 4, 5 * 8 + 3, 4 * 8 + 2,
        };
        Disc turn = Disc::Black;
        for (const int index : line) {
            require(
                board.put(turn, index / 8, index % 8),
                "Could not build opening-book fallback position"
            );
            turn = opposite(turn);
        }
        const auto searchedMove = ai.chooseMove(board, turn);
        require(searchedMove.has_value(), "AI returned no fallback move");
        require(!searchedMove->openingBook, "Book continued beyond stored line");
        require(searchedMove->searchedNodes > 0, "Fallback search visited no nodes");
        require(searchedMove->searchDepth == 3, "Fallback depth is incorrect");

        OthelloAI::SearchProgress timeoutProgress;
        const auto timedOutMove = ai.chooseMove(
            board,
            turn,
            {},
            &timeoutProgress,
            std::chrono::steady_clock::now() - std::chrono::milliseconds(1)
        );
        require(!timedOutMove.has_value(), "Expired search returned a move");
        require(
            timeoutProgress.timedOut.load(std::memory_order_relaxed),
            "Expired search did not report a timeout"
        );

        std::stop_source cancelledSource;
        cancelledSource.request_stop();
        OthelloAI::SearchProgress cancelledProgress;
        const auto cancelledMove = ai.chooseMove(
            board,
            turn,
            cancelledSource.get_token(),
            &cancelledProgress
        );
        require(!cancelledMove.has_value(), "Cancelled search returned a move");
        require(
            !cancelledProgress.timedOut.load(std::memory_order_relaxed),
            "Cancellation was incorrectly reported as a timeout"
        );

        const auto noMove = ai.chooseMove(board, Disc::Empty);
        require(!noMove.has_value(), "AI accepted Disc::Empty");
    }

    void testNeuralMoveOrderingModel() {
        struct ModelCleanup {
            ~ModelCleanup() { std::remove(NeuralTestModelPath); }
        } cleanup;

        std::ofstream output(NeuralTestModelPath, std::ios::binary);
        require(output.good(), "Could not create neural ordering test model");

        const std::uint32_t version = ModelFormat::Version;
        const std::uint32_t parameterCount = 2;
        output.write(ModelFormat::Magic, sizeof(ModelFormat::Magic) - 1);
        output.write(
            reinterpret_cast<const char*>(&version),
            sizeof(version)
        );
        output.write(
            reinterpret_cast<const char*>(&parameterCount),
            sizeof(parameterCount)
        );

        const auto writeMatrix = [&output](
            std::int32_t rows,
            std::int32_t columns,
            const std::vector<float>& values
        ) {
            output.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
            output.write(
                reinterpret_cast<const char*>(&columns),
                sizeof(columns)
            );
            output.write(
                reinterpret_cast<const char*>(values.data()),
                static_cast<std::streamsize>(values.size() * sizeof(float))
            );
        };

        std::vector<float> weights(
            NeuralMoveOrderer::OutputSize * NeuralMoveOrderer::InputSize,
            0.0f
        );
        std::vector<float> biases(NeuralMoveOrderer::OutputSize);
        for (std::size_t index = 0; index < biases.size(); ++index) {
            biases[index] = static_cast<float>(index);
        }
        writeMatrix(64, 128, weights);
        writeMatrix(64, 1, biases);
        output.close();
        require(output.good(), "Could not finish neural ordering test model");

        NeuralMoveOrderer orderer;
        require(
            orderer.Configure(true, NeuralTestModelPath, 4),
            "Valid neural ordering model was rejected"
        );
        require(orderer.IsActive(), "Neural ordering was not activated");

        BitBoard board;
        std::array<float, NeuralMoveOrderer::OutputSize> scores{};
        require(
            !orderer.Score(board, Disc::Black, 3, scores),
            "Neural ordering ignored its minimum depth"
        );
        require(
            orderer.Score(board, Disc::Black, 4, scores),
            "Neural ordering inference failed"
        );
        require(
            scores.front() == 0.0f && scores.back() == 63.0f,
            "Neural ordering returned unexpected move scores"
        );

        require(
            !orderer.Configure(false, NeuralTestModelPath, 4),
            "Disabled neural ordering reported activation"
        );
        require(!orderer.IsActive(), "Neural ordering stayed active");

        OthelloAI ai;
        require(
            !ai.configureNeuralOrdering(
                true,
                "missing_ordering_model.model",
                4
            ),
            "Missing neural ordering model was accepted"
        );
        require(
            !ai.neuralOrderingActive(),
            "Missing model did not fall back to heuristic ordering"
        );
    }

    void testOthelloTrainingData() {
        BitBoard board;
        const auto blackInput = OthelloTrainingData::Encode(
            board,
            Disc::Black
        );
        require(
            blackInput[3 * 8 + 4] == 1.0f &&
            blackInput[4 * 8 + 3] == 1.0f,
            "Training input omitted current-player discs"
        );
        require(
            blackInput[64 + 3 * 8 + 3] == 1.0f &&
            blackInput[64 + 4 * 8 + 4] == 1.0f,
            "Training input omitted opponent discs"
        );

        const auto whiteInput = OthelloTrainingData::Encode(
            board,
            Disc::White
        );
        require(
            whiteInput[3 * 8 + 3] == 1.0f &&
            whiteInput[64 + 3 * 8 + 4] == 1.0f,
            "Training input is not relative to the side to move"
        );

        std::array<bool, 64> transformed{};
        for (
            int symmetry = 0;
            symmetry < OthelloTrainingData::SymmetryCount;
            ++symmetry
        ) {
            const int index = OthelloTrainingData::TransformIndex(
                1,
                symmetry
            );
            require(!transformed[index], "Board symmetry was duplicated");
            transformed[index] = true;
        }

        constexpr const char* CsvPath = "core_test_training.csv";
        {
            OthelloTrainingData::CsvWriter writer;
            require(writer.Open(CsvPath), "Could not create training CSV");
            require(
                writer.Write(board, Disc::Black, 2 * 8 + 3, true),
                "Could not write augmented training data"
            );
            require(
                writer.rowsWritten() == 8,
                "Training data did not emit all board symmetries"
            );
        }
        std::remove(CsvPath);
    }

    void testExactEndgame() {
        BitBoard board;
        Disc turn = Disc::Black;
        std::uint32_t randomState = 0x5eed1234u;

        while (
            64 - board.count(Disc::Black) - board.count(Disc::White) > 6 &&
            !board.isGameOver()
        ) {
            Bits moves = board.legalMoves(turn);
            if (moves == 0) {
                turn = opposite(turn);
                continue;
            }

            const int index = selectMoveIndex(moves, randomState);
            require(
                board.put(
                    turn,
                    index / BitBoard::Size,
                    index % BitBoard::Size
                ),
                "Could not build exact-search test position"
            );
            turn = opposite(turn);
        }

        if (!board.hasAnyMove(turn)) turn = opposite(turn);
        require(board.hasAnyMove(turn), "Endgame position has no move");

        OthelloAI ai(1);
        ai.setExactEndgameEmpty(6);
        const auto move = ai.chooseMove(board, turn);

        require(move.has_value(), "Exact search returned no move");
        require(move->exactSearch, "Endgame did not use exact search");
        require(
            board.canPut(turn, move->row, move->col),
            "Exact search returned an illegal move"
        );

        const int expected = solveExactly(board, turn, turn);
        BitBoard selected = board;
        require(
            selected.put(turn, move->row, move->col),
            "Could not apply exact-search result"
        );
        const int actual = solveExactly(selected, opposite(turn), turn);
        require(actual == expected, "Exact search did not choose an optimal move");
    }

    void testSearchStatisticsLogger() {
        constexpr const char* CsvPath = "core_test_search_statistics.csv";
        std::remove(CsvPath);

        SearchStatisticsEntry entry;
        entry.difficultyName = "TEST,NN";
        entry.black = 0x0000000810000000ULL;
        entry.white = 0x0000001008000000ULL;
        entry.turn = Disc::Black;
        entry.configuredDepth = 10;
        entry.targetDepth = 10;
        entry.completedDepth = 8;
        entry.emptyCount = 60;
        entry.legalMoveCount = 4;
        entry.timeLimitMs = 10'000;
        entry.elapsedMs = 2'000;
        entry.searchedNodes = 50'000;
        entry.transpositionHits = 2'500;
        entry.timedOut = true;
        entry.neuralOrderingEnabled = true;
        entry.neuralOrderingActive = true;
        entry.neuralOrderingMinimumDepth = 4;
        entry.moveRow = 2;
        entry.moveCol = 3;
        entry.score = 42;

        require(
            SearchStatisticsLogger::Append(CsvPath, entry),
            "Could not write search statistics"
        );
        require(
            SearchStatisticsLogger::Append(CsvPath, entry),
            "Could not append search statistics"
        );

        std::ifstream input(CsvPath);
        require(input.good(), "Could not reopen search statistics");
        std::vector<std::string> lines;
        for (std::string line; std::getline(input, line);) {
            lines.push_back(std::move(line));
        }
        require(lines.size() == 3, "Search statistics header was duplicated");
        require(
            lines[1].find("\"TEST,NN\"") != std::string::npos,
            "Search statistics did not escape CSV text"
        );
        require(
            lines[1].find(",2000,50000,25000.00,2500,5.00,") !=
                std::string::npos,
            "Search statistics calculated rates incorrectly"
        );
        input.close();
        std::remove(CsvPath);
    }
}

int main() {
    try {
        testInitialPosition();
        testRandomGamesAndPasses();
        testLegacyOpeningBook();
        testAiReturnsLegalMove();
        testNeuralMoveOrderingModel();
        testOthelloTrainingData();
        testExactEndgame();
        testSearchStatisticsLogger();
        std::cout << "All core tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TEST FAILED: " << error.what() << '\n';
        return 1;
    }
}
