#include "ai/OthelloAI.h"

#include <algorithm>
#include <array>
#include <bit>

namespace {
    constexpr std::array<int, 64> PositionWeights = {
         120, -25,  20,   5,   5,  20, -25, 120,
         -25, -45,  -5,  -5,  -5,  -5, -45, -25,
          20,  -5,  15,   3,   3,  15,  -5,  20,
           5,  -5,   3,   3,   3,   3,  -5,   5,
           5,  -5,   3,   3,   3,   3,  -5,   5,
          20,  -5,  15,   3,   3,  15,  -5,  20,
         -25, -45,  -5,  -5,  -5,  -5, -45, -25,
         120, -25,  20,   5,   5,  20, -25, 120,
    };
}

OthelloAI::OthelloAI(int depth) noexcept {
    setDepth(depth);
}

void OthelloAI::setDepth(int depth) noexcept {
    depth_ = std::clamp(depth, 1, 12);
}

std::optional<OthelloAI::Move> OthelloAI::chooseMove(
    const BitBoard& board,
    Disc disc
) const {
    if (disc == Disc::Empty || !board.hasAnyMove(disc)) {
        return std::nullopt;
    }

    searchedNodes_ = 0;
    auto moves = orderedMoves(board, disc);

    Move bestMove = moves.front();
    int alpha = -Infinity;
    const int beta = Infinity;

    for (const Move& move : moves) {
        BitBoard child = board;
        if (!child.put(disc, move.row, move.col)) continue;

        const int score = -negamax(
            child,
            opponentOf(disc),
            depth_ - 1,
            -beta,
            -alpha
        );

        if (score > bestMove.score || bestMove.row < 0) {
            bestMove = move;
            bestMove.score = score;
        }
        alpha = std::max(alpha, score);
    }

    bestMove.searchedNodes = searchedNodes_;
    return bestMove;
}

int OthelloAI::negamax(
    const BitBoard& board,
    Disc turn,
    int depth,
    int alpha,
    int beta
) const {
    ++searchedNodes_;

    const Disc opponent = opponentOf(turn);
    const bool canMove = board.hasAnyMove(turn);

    if (!canMove) {
        if (!board.hasAnyMove(opponent)) {
            return terminalScore(board, turn);
        }
        return -negamax(board, opponent, depth, -beta, -alpha);
    }

    if (depth <= 0) {
        return evaluate(board, turn);
    }

    int best = -Infinity;
    const auto moves = orderedMoves(board, turn);

    for (const Move& move : moves) {
        BitBoard child = board;
        if (!child.put(turn, move.row, move.col)) continue;

        const int score = -negamax(
            child,
            opponent,
            depth - 1,
            -beta,
            -alpha
        );

        best = std::max(best, score);
        alpha = std::max(alpha, score);
        if (alpha >= beta) break;
    }

    return best;
}

int OthelloAI::evaluate(const BitBoard& board, Disc perspective) const {
    const Disc opponent = opponentOf(perspective);
    const BitBoard::Bits mine =
        perspective == Disc::Black ? board.black() : board.white();
    const BitBoard::Bits theirs =
        opponent == Disc::Black ? board.black() : board.white();

    int positionScore = 0;
    for (int index = 0; index < 64; ++index) {
        const BitBoard::Bits bit = static_cast<BitBoard::Bits>(1) << index;
        if ((mine & bit) != 0) {
            positionScore += PositionWeights[index];
        } else if ((theirs & bit) != 0) {
            positionScore -= PositionWeights[index];
        }
    }

    const int mobility =
        std::popcount(board.legalMoves(perspective)) -
        std::popcount(board.legalMoves(opponent));

    const int discDifference =
        board.count(perspective) - board.count(opponent);
    const int occupied = board.count(Disc::Black) + board.count(Disc::White);
    const int discWeight = occupied < 20 ? 1 : (occupied < 50 ? 3 : 10);

    return positionScore * 4 + mobility * 12 + discDifference * discWeight;
}

int OthelloAI::terminalScore(
    const BitBoard& board,
    Disc perspective
) const {
    const Disc opponent = opponentOf(perspective);
    const int difference =
        board.count(perspective) - board.count(opponent);

    if (difference > 0) return WinScore + difference;
    if (difference < 0) return -WinScore + difference;
    return 0;
}

std::vector<OthelloAI::Move> OthelloAI::orderedMoves(
    const BitBoard& board,
    Disc disc
) const {
    std::vector<Move> result;
    BitBoard::Bits moves = board.legalMoves(disc);
    result.reserve(std::popcount(moves));

    while (moves != 0) {
        const int index = std::countr_zero(moves);
        result.push_back({
            index / BitBoard::Size,
            index % BitBoard::Size,
            PositionWeights[index],
            0,
        });
        moves &= moves - 1;
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const Move& lhs, const Move& rhs) {
            return lhs.score > rhs.score;
        }
    );
    return result;
}

Disc OthelloAI::opponentOf(Disc disc) noexcept {
    if (disc == Disc::Black) return Disc::White;
    if (disc == Disc::White) return Disc::Black;
    return Disc::Empty;
}
