#include "ai/OthelloAI.h"

#include <algorithm>
#include <array>
#include <bit>

namespace {
    struct SearchCancelled {};

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

    std::uint64_t mixHash(std::uint64_t value) noexcept {
        value ^= value >> 30;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27;
        value *= 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }
}

void OthelloAI::SearchProgress::reset(int target, bool exact) noexcept {
    searchedNodes.store(0, std::memory_order_relaxed);
    transpositionHits.store(0, std::memory_order_relaxed);
    completedDepth.store(0, std::memory_order_relaxed);
    targetDepth.store(target, std::memory_order_relaxed);
    exactSearch.store(exact, std::memory_order_relaxed);
}

OthelloAI::OthelloAI(int depth) noexcept {
    setDepth(depth);
}

void OthelloAI::setDepth(int depth) noexcept {
    depth_ = std::clamp(depth, 1, 12);
}

void OthelloAI::setExactEndgameEmpty(int emptyCount) noexcept {
    exactEndgameEmpty_ = std::clamp(emptyCount, 0, 20);
}

std::optional<OthelloAI::Move> OthelloAI::chooseMove(
    const BitBoard& board,
    Disc disc,
    std::stop_token stopToken,
    SearchProgress* progress
) const {
    if (disc == Disc::Empty || !board.hasAnyMove(disc)) {
        return std::nullopt;
    }

    const int emptyCount = 64 - board.count(Disc::Black) - board.count(Disc::White);
    const bool exactSearch = emptyCount <= exactEndgameEmpty_;
    const int searchDepth = exactSearch ? emptyCount : depth_;

    stopToken_ = stopToken;
    progress_ = progress;
    if (progress_) progress_->reset(searchDepth, exactSearch);

    searchedNodes_ = 0;
    transpositionHits_ = 0;
    transpositionTable_.clear();
    transpositionTable_.reserve(MaxTranspositionEntries);

    Move bestMove;
    try {
        checkCancellation();
        int preferredMoveIndex = -1;
        for (int iterationDepth = 1; iterationDepth <= searchDepth; ++iterationDepth) {
            bestMove = searchRoot(
                board, disc, iterationDepth, exactSearch, preferredMoveIndex
            );
            preferredMoveIndex = bestMove.row * BitBoard::Size + bestMove.col;
            bestMove.completedIterations = iterationDepth;
            if (progress_) {
                progress_->completedDepth.store(
                    iterationDepth, std::memory_order_relaxed
                );
            }
            publishProgress();
        }
    } catch (const SearchCancelled&) {
        publishProgress();
        progress_ = nullptr;
        return std::nullopt;
    }

    bestMove.searchedNodes = searchedNodes_;
    bestMove.searchDepth = searchDepth;
    bestMove.exactSearch = exactSearch;
    bestMove.transpositionHits = transpositionHits_;
    publishProgress();
    progress_ = nullptr;
    return bestMove;
}

OthelloAI::Move OthelloAI::searchRoot(
    const BitBoard& board,
    Disc disc,
    int depth,
    bool exactSearch,
    int preferredMoveIndex
) const {
    auto moves = orderedMoves(board, disc, preferredMoveIndex);
    Move bestMove;
    int alpha = -Infinity;
    const int beta = Infinity;

    bool firstMove = true;
    for (const Move& move : moves) {
        BitBoard child = board;
        if (!child.put(disc, move.row, move.col)) continue;

        int score;
        if (firstMove) {
            score = -negaScout(
                child, opponentOf(disc), depth - 1,
                -beta, -alpha, exactSearch
            );
            firstMove = false;
        } else {
            score = -negaScout(
                child, opponentOf(disc), depth - 1,
                -alpha - 1, -alpha, exactSearch
            );
            if (alpha < score && score < beta) {
                score = -negaScout(
                    child, opponentOf(disc), depth - 1,
                    -beta, -alpha, exactSearch
                );
            }
        }

        if (score > bestMove.score || bestMove.row < 0) {
            bestMove = move;
            bestMove.score = score;
        }
        alpha = std::max(alpha, score);
    }

    return bestMove;
}

int OthelloAI::negaScout(
    const BitBoard& board,
    Disc turn,
    int depth,
    int alpha,
    int beta,
    bool exactSearch
) const {
    ++searchedNodes_;
    if ((searchedNodes_ & 0x0fffULL) == 0) {
        checkCancellation();
        publishProgress();
    }

    const int originalAlpha = alpha;
    const int originalBeta = beta;
    const PositionKey key{ board.black(), board.white(), turn };
    int preferredMoveIndex = -1;

    const auto found = transpositionTable_.find(key);
    if (found != transpositionTable_.end()) {
        preferredMoveIndex = found->second.bestMoveIndex;
        if (found->second.depth >= depth) {
            ++transpositionHits_;
            if (found->second.bound == Bound::Exact) {
                return found->second.score;
            }
            if (found->second.bound == Bound::Lower) {
                alpha = std::max(alpha, found->second.score);
            } else {
                beta = std::min(beta, found->second.score);
            }
            if (alpha >= beta) return found->second.score;
        }
    }

    const Disc opponent = opponentOf(turn);
    const bool canMove = board.hasAnyMove(turn);

    if (!canMove) {
        if (!board.hasAnyMove(opponent)) {
            const int score = terminalScore(board, turn);
            storeTransposition(key, { depth, score, -1, Bound::Exact });
            return score;
        }
        const int score = -negaScout(
            board, opponent, depth, -beta, -alpha, exactSearch
        );
        Bound bound = Bound::Exact;
        if (score <= originalAlpha) bound = Bound::Upper;
        else if (score >= originalBeta) bound = Bound::Lower;
        storeTransposition(key, { depth, score, -1, bound });
        return score;
    }

    if (depth <= 0) {
        const int score = evaluate(board, turn);
        storeTransposition(key, { depth, score, -1, Bound::Exact });
        return score;
    }

    const auto moves = orderedMoves(board, turn, preferredMoveIndex);
    bool firstMove = true;
    int bestMoveIndex = -1;
    for (const Move& move : moves) {
        BitBoard child = board;
        if (!child.put(turn, move.row, move.col)) continue;

        int score;
        if (firstMove) {
            score = -negaScout(
                child, opponent, depth - 1, -beta, -alpha, exactSearch
            );
            firstMove = false;
        } else {
            score = -negaScout(
                child, opponent, depth - 1, -alpha - 1, -alpha, exactSearch
            );
            if (alpha < score && score < beta) {
                score = -negaScout(
                    child, opponent, depth - 1, -beta, -alpha, exactSearch
                );
            }
        }

        if (score > alpha) {
            alpha = score;
            bestMoveIndex = move.row * BitBoard::Size + move.col;
        }
        if (alpha >= beta) break;
    }

    Bound bound = Bound::Exact;
    if (alpha <= originalAlpha) bound = Bound::Upper;
    else if (alpha >= originalBeta) bound = Bound::Lower;
    storeTransposition(key, { depth, alpha, bestMoveIndex, bound });
    return alpha;
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
    Disc disc,
    int preferredMoveIndex
) const {
    std::vector<Move> result;
    BitBoard::Bits moves = board.legalMoves(disc);
    result.reserve(std::popcount(moves));

    while (moves != 0) {
        const int index = std::countr_zero(moves);
        result.push_back({
            index / BitBoard::Size,
            index % BitBoard::Size,
            0,
            0,
            0,
            false,
            0,
            0,
        });
        BitBoard child = board;
        child.put(disc, index / BitBoard::Size, index % BitBoard::Size);
        result.back().score = PositionWeights[index] * 100
            - std::popcount(child.legalMoves(opponentOf(disc))) * 10;
        moves &= moves - 1;
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const Move& lhs, const Move& rhs) {
            return lhs.score > rhs.score;
        }
    );
    if (preferredMoveIndex >= 0) {
        const auto preferred = std::find_if(
            result.begin(), result.end(),
            [preferredMoveIndex](const Move& move) {
                return move.row * BitBoard::Size + move.col == preferredMoveIndex;
            }
        );
        if (preferred != result.end()) {
            std::rotate(result.begin(), preferred, preferred + 1);
        }
    }
    return result;
}

std::size_t OthelloAI::PositionKeyHash::operator()(
    const PositionKey& key
) const noexcept {
    const std::uint64_t turnSalt =
        key.turn == Disc::Black ? 0x9e3779b97f4a7c15ULL
        : 0x243f6a8885a308d3ULL;
    return static_cast<std::size_t>(
        mixHash(key.black) ^ (mixHash(key.white) << 1) ^ turnSalt
    );
}

void OthelloAI::storeTransposition(
    const PositionKey& key,
    const TranspositionEntry& entry
) const {
    const auto found = transpositionTable_.find(key);
    if (found != transpositionTable_.end()) {
        if (entry.depth >= found->second.depth) found->second = entry;
        return;
    }
    if (transpositionTable_.size() < MaxTranspositionEntries) {
        transpositionTable_.emplace(key, entry);
    }
}

void OthelloAI::checkCancellation() const {
    if (stopToken_.stop_requested()) throw SearchCancelled{};
}

void OthelloAI::publishProgress() const noexcept {
    if (!progress_) return;
    progress_->searchedNodes.store(searchedNodes_, std::memory_order_relaxed);
    progress_->transpositionHits.store(
        transpositionHits_, std::memory_order_relaxed
    );
}

Disc OthelloAI::opponentOf(Disc disc) noexcept {
    if (disc == Disc::Black) return Disc::White;
    if (disc == Disc::White) return Disc::Black;
    return Disc::Empty;
}
