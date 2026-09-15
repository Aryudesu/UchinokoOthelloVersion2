#pragma once

#include <cstdint>
#include <optional>

enum class Disc : std::uint8_t {
    Empty,
    Black,
    White,
};

class BitBoard {
public:
    using Bits = std::uint64_t;
    static constexpr int Size = 8;

    BitBoard();

    [[nodiscard]] static std::optional<BitBoard> FromBits(
        Bits black,
        Bits white
    ) noexcept;

    void reset();

    [[nodiscard]] Bits black() const noexcept { return black_; }
    [[nodiscard]] Bits white() const noexcept { return white_; }
    [[nodiscard]] Disc discAt(int row, int col) const noexcept;

    [[nodiscard]] Bits flipsFor(Disc disc, int row, int col) const noexcept;
    [[nodiscard]] Bits legalMoves(Disc disc) const noexcept;
    [[nodiscard]] bool canPut(Disc disc, int row, int col) const noexcept;
    bool put(Disc disc, int row, int col) noexcept;

    [[nodiscard]] bool hasAnyMove(Disc disc) const noexcept;
    [[nodiscard]] bool isFull() const noexcept;
    [[nodiscard]] bool isGameOver() const noexcept;
    [[nodiscard]] int count(Disc disc) const noexcept;

    [[nodiscard]] static constexpr Bits bitAt(int row, int col) noexcept {
        return static_cast<Bits>(1) << (row * Size + col);
    }

private:
    Bits black_ = 0;
    Bits white_ = 0;

    [[nodiscard]] static bool isInside(int row, int col) noexcept;
    [[nodiscard]] static Disc opponentOf(Disc disc) noexcept;
    [[nodiscard]] Bits bitsOf(Disc disc) const noexcept;
};
