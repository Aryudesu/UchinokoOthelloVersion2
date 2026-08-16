#include "model/BitBoard.h"

#include <bit>

namespace {
    constexpr int Directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        { 0, -1},           { 0, 1},
        { 1, -1}, { 1, 0}, { 1, 1},
    };
}

BitBoard::BitBoard() {
    reset();
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
    if (disc == Disc::Empty || !isInside(row, col) || discAt(row, col) != Disc::Empty) {
        return 0;
    }

    const Disc opponent = opponentOf(disc);
    Bits result = 0;

    for (const auto& direction : Directions) {
        int currentRow = row + direction[0];
        int currentCol = col + direction[1];
        Bits candidates = 0;

        while (isInside(currentRow, currentCol) &&
               discAt(currentRow, currentCol) == opponent) {
            candidates |= bitAt(currentRow, currentCol);
            currentRow += direction[0];
            currentCol += direction[1];
        }

        if (candidates != 0 &&
            isInside(currentRow, currentCol) &&
            discAt(currentRow, currentCol) == disc) {
            result |= candidates;
        }
    }

    return result;
}

BitBoard::Bits BitBoard::legalMoves(Disc disc) const noexcept {
    if (disc == Disc::Empty) return 0;

    Bits result = 0;
    for (int row = 0; row < Size; ++row) {
        for (int col = 0; col < Size; ++col) {
            if (flipsFor(disc, row, col) != 0) {
                result |= bitAt(row, col);
            }
        }
    }
    return result;
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
