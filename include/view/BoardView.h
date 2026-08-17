#pragma once

#include "model/BitBoard.h"

class BoardView {
public:
    static constexpr int CellSize = 40;
    static constexpr int BoardSize = BitBoard::Size;
    static constexpr int BoardLeft = 200;
    static constexpr int BoardTop = 80;

    void Start();
    void End();
    void Reset(const BitBoard& board);
    void Update();

    void BeginMove(
        const BitBoard& before,
        const BitBoard& after,
        Disc placedDisc,
        int row,
        int col
    );

    void Draw(
        const BitBoard& board,
        BitBoard::Bits legalMoves,
        bool showLegalMoves
    ) const;

    [[nodiscard]] bool HitTest(int mouseX, int mouseY, int& row, int& col) const noexcept;
    [[nodiscard]] bool IsAnimating() const noexcept { return animatedBits_ != 0; }

private:
    // stone.bmp indices. Keeping these names here makes a future 9/10 swap local.
    static constexpr int StoneEmpty = 0;
    static constexpr int StoneBlack = 1;
    static constexpr int StoneWhite = 2;
    static constexpr int StoneBlackToWhiteFirst = 3;
    static constexpr int StoneWhiteToBlackFirst = 6;
    static constexpr int StoneLegalMove = 9;
    static constexpr int StoneLastMove = 10;

    static constexpr int BoardFrameCount = 12;
    static constexpr int BoardFrameDurationMs = 270;
    static constexpr int FlipFrameCount = 3;
    static constexpr int FlipFrameDurationMs = 80;

    BitBoard beforeMove_;
    BitBoard::Bits animatedBits_ = 0;
    int animationStartedAt_ = 0;
    int lastMoveRow_ = -1;
    int lastMoveCol_ = -1;
    bool loaded_ = false;

    [[nodiscard]] int stoneFrameAt(
        const BitBoard& board,
        int row,
        int col,
        int elapsedMs
    ) const noexcept;
};
