#pragma once

#include "model/BitBoard.h"

#include <cstdint>
#include <optional>
#include <string_view>
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

    [[nodiscard]] std::u8string_view completedName(
        const BitBoard& board,
        Disc turn
    ) const noexcept;

    [[nodiscard]] std::size_t positionCount() const noexcept {
        return entries_.size();
    }
    [[nodiscard]] std::size_t lineCount() const noexcept {
        return lineCount_;
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
    std::unordered_map<PositionKey, std::u8string_view, PositionKeyHash>
        completedNames_;
    std::size_t lineCount_ = 0;

    [[nodiscard]] bool addNotationLine(
        std::string_view notation,
        std::u8string_view name
    );
    [[nodiscard]] bool addLine(const std::vector<int>& moves);
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
