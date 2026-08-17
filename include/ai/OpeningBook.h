#pragma once

#include "model/BitBoard.h"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

class OpeningBook {
public:
    struct Move {
        int row = -1;
        int col = -1;
    };

    OpeningBook();

    [[nodiscard]] std::optional<Move> findMove(
        const BitBoard& board,
        Disc turn
    ) const;

    [[nodiscard]] std::size_t positionCount() const noexcept {
        return entries_.size();
    }

private:
    struct PositionKey {
        BitBoard::Bits black = 0;
        BitBoard::Bits white = 0;
        Disc turn = Disc::Empty;

        bool operator==(const PositionKey&) const noexcept = default;
    };

    struct PositionKeyHash {
        [[nodiscard]] std::size_t operator()(
            const PositionKey& key
        ) const noexcept;
    };

    struct CanonicalPosition {
        PositionKey key;
        int transform = 0;
    };

    std::unordered_map<PositionKey, std::vector<int>, PositionKeyHash> entries_;

    void addLine(const std::vector<int>& moves);
    [[nodiscard]] static CanonicalPosition canonicalize(
        const BitBoard& board,
        Disc turn
    ) noexcept;
    [[nodiscard]] static BitBoard::Bits transformBits(
        BitBoard::Bits bits,
        int transform
    ) noexcept;
    [[nodiscard]] static int transformIndex(
        int index,
        int transform
    ) noexcept;
    [[nodiscard]] static int inverseTransformIndex(
        int index,
        int transform
    ) noexcept;
    [[nodiscard]] static Disc opponentOf(Disc disc) noexcept;
};
