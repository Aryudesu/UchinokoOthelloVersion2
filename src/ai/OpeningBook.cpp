#include "ai/OpeningBook.h"

#include <algorithm>
#include <array>
#include <bit>
#include <utility>

namespace {
    constexpr int square(char file, int rank) noexcept {
        return (rank - 1) * BitBoard::Size + (file - 'a');
    }

    std::uint64_t mixHash(std::uint64_t value) noexcept {
        value ^= value >> 30;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27;
        value *= 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }
}

OpeningBook::OpeningBook() {
    // A compact set of established opening branches. Symmetric positions are
    // normalized, so each line also covers every rotation and reflection.
    addLine({
        square('f', 5), square('d', 6), square('c', 3),
        square('d', 3), square('c', 4), square('f', 4),
        square('f', 6), square('f', 3), square('e', 3),
    });
    addLine({
        square('f', 5), square('f', 6), square('e', 6),
        square('f', 4), square('e', 3), square('d', 6),
        square('c', 5),
    });
    addLine({
        square('d', 3), square('c', 3), square('c', 4),
        square('e', 3), square('f', 4), square('c', 5),
    });
    addLine({
        square('c', 4), square('c', 3), square('d', 3),
        square('c', 5), square('b', 4),
    });
    addLine({
        square('e', 6), square('f', 6), square('f', 5),
        square('d', 6), square('c', 5), square('f', 4),
    });
}

std::optional<OpeningBook::Move> OpeningBook::findMove(
    const BitBoard& board,
    Disc turn
) const {
    if (turn == Disc::Empty) return std::nullopt;

    const CanonicalPosition canonical = canonicalize(board, turn);
    const auto found = entries_.find(canonical.key);
    if (found == entries_.end()) return std::nullopt;

    for (const int canonicalIndex : found->second) {
        const int index = inverseTransformIndex(
            canonicalIndex, canonical.transform
        );
        const int row = index / BitBoard::Size;
        const int col = index % BitBoard::Size;
        if (board.canPut(turn, row, col)) return Move{ row, col };
    }
    return std::nullopt;
}

void OpeningBook::addLine(const std::vector<int>& moves) {
    BitBoard board;
    Disc turn = Disc::Black;

    for (const int moveIndex : moves) {
        const int row = moveIndex / BitBoard::Size;
        const int col = moveIndex % BitBoard::Size;
        if (!board.canPut(turn, row, col)) return;

        const CanonicalPosition canonical = canonicalize(board, turn);
        const int canonicalMove = transformIndex(
            moveIndex, canonical.transform
        );
        auto& candidates = entries_[canonical.key];
        if (std::find(candidates.begin(), candidates.end(), canonicalMove)
            == candidates.end()) {
            candidates.push_back(canonicalMove);
        }

        board.put(turn, row, col);
        turn = opponentOf(turn);
    }
}

OpeningBook::CanonicalPosition OpeningBook::canonicalize(
    const BitBoard& board,
    Disc turn
) noexcept {
    CanonicalPosition best{
        { board.black(), board.white(), turn },
        0,
    };

    for (int transform = 1; transform < 8; ++transform) {
        const PositionKey candidate{
            transformBits(board.black(), transform),
            transformBits(board.white(), transform),
            turn,
        };
        if (std::pair{ candidate.black, candidate.white }
            < std::pair{ best.key.black, best.key.white }) {
            best = { candidate, transform };
        }
    }
    return best;
}

BitBoard::Bits OpeningBook::transformBits(
    BitBoard::Bits bits,
    int transform
) noexcept {
    BitBoard::Bits result = 0;
    while (bits != 0) {
        const int index = std::countr_zero(bits);
        result |= static_cast<BitBoard::Bits>(1)
            << transformIndex(index, transform);
        bits &= bits - 1;
    }
    return result;
}

int OpeningBook::transformIndex(int index, int transform) noexcept {
    const int row = index / BitBoard::Size;
    const int col = index % BitBoard::Size;
    const int last = BitBoard::Size - 1;

    switch (transform) {
    case 1: return col * BitBoard::Size + (last - row);
    case 2: return (last - row) * BitBoard::Size + (last - col);
    case 3: return (last - col) * BitBoard::Size + row;
    case 4: return row * BitBoard::Size + (last - col);
    case 5: return (last - col) * BitBoard::Size + (last - row);
    case 6: return (last - row) * BitBoard::Size + col;
    case 7: return col * BitBoard::Size + row;
    default: return index;
    }
}

int OpeningBook::inverseTransformIndex(
    int index,
    int transform
) noexcept {
    for (int original = 0; original < 64; ++original) {
        if (transformIndex(original, transform) == index) return original;
    }
    return index;
}

Disc OpeningBook::opponentOf(Disc disc) noexcept {
    if (disc == Disc::Black) return Disc::White;
    if (disc == Disc::White) return Disc::Black;
    return Disc::Empty;
}

std::size_t OpeningBook::PositionKeyHash::operator()(
    const PositionKey& key
) const noexcept {
    const std::uint64_t turnSalt =
        key.turn == Disc::Black ? 0x9e3779b97f4a7c15ULL
        : 0x243f6a8885a308d3ULL;
    return static_cast<std::size_t>(
        mixHash(key.black) ^ (mixHash(key.white) << 1) ^ turnSalt
    );
}
