#include "model/BitBoard.h"

#include <bit>

namespace {
    using Bits = BitBoard::Bits;

    constexpr Bits NotAFile = 0xfefefefefefefefeULL;
    constexpr Bits NotHFile = 0x7f7f7f7f7f7f7f7fULL;

    enum class Direction {
        North,
        NorthEast,
        East,
        SouthEast,
        South,
        SouthWest,
        West,
        NorthWest,
    };

    template <Direction direction>
    constexpr Bits shift(Bits bits) noexcept {
        if constexpr (direction == Direction::North) {
            return bits >> 8;
        } else if constexpr (direction == Direction::NorthEast) {
            return (bits & NotHFile) >> 7;
        } else if constexpr (direction == Direction::East) {
            return (bits & NotHFile) << 1;
        } else if constexpr (direction == Direction::SouthEast) {
            return (bits & NotHFile) << 9;
        } else if constexpr (direction == Direction::South) {
            return bits << 8;
        } else if constexpr (direction == Direction::SouthWest) {
            return (bits & NotAFile) << 7;
        } else if constexpr (direction == Direction::West) {
            return (bits & NotAFile) >> 1;
        } else {
            return (bits & NotAFile) >> 9;
        }
    }

    template <Direction direction>
    Bits flipsInDirection(Bits move, Bits mine, Bits opponent) noexcept {
        Bits flipped = 0;
        Bits cursor = shift<direction>(move) & opponent;

        while (cursor != 0) {
            flipped |= cursor;
            const Bits next = shift<direction>(cursor);
            if ((next & mine) != 0) {
                return flipped;
            }
            cursor = next & opponent;
        }
        return 0;
    }

    template <Direction direction>
    Bits legalMovesInDirection(Bits mine, Bits opponent, Bits empty) noexcept {
        Bits candidates = shift<direction>(mine) & opponent;

        // 1方向に並べる相手石は最大6個。
        // 盤面全体を同時に5回伝播させ、すべての長さを拾う。
        candidates |= shift<direction>(candidates) & opponent;
        candidates |= shift<direction>(candidates) & opponent;
        candidates |= shift<direction>(candidates) & opponent;
        candidates |= shift<direction>(candidates) & opponent;
        candidates |= shift<direction>(candidates) & opponent;

        return shift<direction>(candidates) & empty;
    }
}

BitBoard::BitBoard() {
    reset();
}

std::optional<BitBoard> BitBoard::FromBits(Bits black, Bits white) noexcept {
    if ((black & white) != 0) return std::nullopt;
    BitBoard board;
    board.black_ = black;
    board.white_ = white;
    return board;
}

void BitBoard::reset() {
    black_ = bitAt(3, 4) | bitAt(4, 3);
    white_ = bitAt(3, 3) | bitAt(4, 4);
}

Disc BitBoard::discAt(int row, int col) const noexcept {
    if (!isInside(row, col)) return Disc::Empty;

    const Bits bit = bitAt(row, col);
    if ((black_ & bit) != 0) return Disc::Black;
    if ((white_ & bit) != 0) return Disc::White;
    return Disc::Empty;
}

BitBoard::Bits BitBoard::flipsFor(Disc disc, int row, int col) const noexcept {
    if (disc == Disc::Empty || !isInside(row, col)) return 0;

    const Bits move = bitAt(row, col);
    if (((black_ | white_) & move) != 0) return 0;

    const Bits mine = bitsOf(disc);
    const Bits opponent = bitsOf(opponentOf(disc));

    return
        flipsInDirection<Direction::North>(move, mine, opponent) |
        flipsInDirection<Direction::NorthEast>(move, mine, opponent) |
        flipsInDirection<Direction::East>(move, mine, opponent) |
        flipsInDirection<Direction::SouthEast>(move, mine, opponent) |
        flipsInDirection<Direction::South>(move, mine, opponent) |
        flipsInDirection<Direction::SouthWest>(move, mine, opponent) |
        flipsInDirection<Direction::West>(move, mine, opponent) |
        flipsInDirection<Direction::NorthWest>(move, mine, opponent);
}

BitBoard::Bits BitBoard::legalMoves(Disc disc) const noexcept {
    if (disc == Disc::Empty) return 0;

    const Bits mine = bitsOf(disc);
    const Bits opponent = bitsOf(opponentOf(disc));
    const Bits empty = ~(mine | opponent);

    return
        legalMovesInDirection<Direction::North>(mine, opponent, empty) |
        legalMovesInDirection<Direction::NorthEast>(mine, opponent, empty) |
        legalMovesInDirection<Direction::East>(mine, opponent, empty) |
        legalMovesInDirection<Direction::SouthEast>(mine, opponent, empty) |
        legalMovesInDirection<Direction::South>(mine, opponent, empty) |
        legalMovesInDirection<Direction::SouthWest>(mine, opponent, empty) |
        legalMovesInDirection<Direction::West>(mine, opponent, empty) |
        legalMovesInDirection<Direction::NorthWest>(mine, opponent, empty);
}

bool BitBoard::canPut(Disc disc, int row, int col) const noexcept {
    return flipsFor(disc, row, col) != 0;
}

bool BitBoard::put(Disc disc, int row, int col) noexcept {
    const Bits flips = flipsFor(disc, row, col);
    if (flips == 0) return false;

    const Bits placed = bitAt(row, col);
    Bits& mine = disc == Disc::Black ? black_ : white_;
    Bits& opponent = disc == Disc::Black ? white_ : black_;

    mine |= placed | flips;
    opponent &= ~flips;
    return true;
}

bool BitBoard::hasAnyMove(Disc disc) const noexcept {
    return legalMoves(disc) != 0;
}

bool BitBoard::isFull() const noexcept {
    return (black_ | white_) == ~static_cast<Bits>(0);
}

bool BitBoard::isGameOver() const noexcept {
    return isFull() ||
        (!hasAnyMove(Disc::Black) && !hasAnyMove(Disc::White));
}

int BitBoard::count(Disc disc) const noexcept {
    return std::popcount(bitsOf(disc));
}

bool BitBoard::isInside(int row, int col) noexcept {
    return 0 <= row && row < Size && 0 <= col && col < Size;
}

Disc BitBoard::opponentOf(Disc disc) noexcept {
    if (disc == Disc::Black) return Disc::White;
    if (disc == Disc::White) return Disc::Black;
    return Disc::Empty;
}

BitBoard::Bits BitBoard::bitsOf(Disc disc) const noexcept {
    if (disc == Disc::Black) return black_;
    if (disc == Disc::White) return white_;
    return 0;
}
