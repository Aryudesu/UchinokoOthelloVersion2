#include "view/BoardView.h"

#include "core/Ids.h"
#include "manager/ImageManager.h"
#include "DxLib.h"

#include <algorithm>

namespace {
    constexpr int SheetColumns = 3;
    constexpr int SheetRows = 4;
    constexpr int SheetFrameSize = 40;

    constexpr int FrameTopLeft = 0;
    constexpr int FrameTop = 1;
    constexpr int FrameTopRight = 2;
    constexpr int FrameLeft = 3;
    constexpr int FrameRight = 5;
    constexpr int FrameBottomLeft = 6;
    constexpr int FrameBottom = 7;
    constexpr int FrameBottomRight = 8;
}

void BoardView::Start() {
    auto& images = ImageManager::GetInstance();

    images.LoadDiv(
        ImageID::Board,
        SheetFrameSize,
        SheetFrameSize,
        SheetColumns,
        SheetRows,
        "data/img/board.bmp"
    );

    images.SetTrans(163, 73, 164);
    images.LoadDiv(
        ImageID::Frame,
        SheetFrameSize,
        SheetFrameSize,
        SheetColumns,
        SheetRows,
        "data/img/frame.bmp"
    );

    images.SetTrans(163, 73, 164);
    images.LoadDiv(
        ImageID::Stone,
        SheetFrameSize,
        SheetFrameSize,
        SheetColumns,
        SheetRows,
        "data/img/stone.bmp"
    );

    loaded_ = true;
}

void BoardView::End() {
    if (!loaded_) return;

    auto& images = ImageManager::GetInstance();
    images.Destroy(ImageID::Stone);
    images.Destroy(ImageID::Board);
    images.Destroy(ImageID::Frame);
    loaded_ = false;
}

void BoardView::Reset(const BitBoard& board) {
    beforeMove_ = board;
    animatedBits_ = 0;
    animationStartedAt_ = 0;
    lastMoveRow_ = -1;
    lastMoveCol_ = -1;
}

void BoardView::Update() {
    if (animatedBits_ == 0) return;

    const int elapsed = GetNowCount() - animationStartedAt_;
    if (elapsed >= FlipFrameCount * FlipFrameDurationMs) {
        animatedBits_ = 0;
    }
}

void BoardView::BeginMove(
    const BitBoard& before,
    const BitBoard& after,
    Disc placedDisc,
    int row,
    int col
) {
    beforeMove_ = before;
    animatedBits_ = (before.black() ^ after.black()) |
                    (before.white() ^ after.white());

    // The placed square has no dedicated transition frames. It appears at once;
    // only discs that actually change colour use frames 3..8.
    animatedBits_ &= ~BitBoard::bitAt(row, col);

    animationStartedAt_ = GetNowCount();
    lastMoveRow_ = row;
    lastMoveCol_ = col;

    (void)placedDisc;
}

int BoardView::stoneFrameAt(
    const BitBoard& board,
    int row,
    int col,
    int elapsedMs
) const noexcept {
    const Disc current = board.discAt(row, col);
    const BitBoard::Bits bit = BitBoard::bitAt(row, col);

    if ((animatedBits_ & bit) != 0) {
        const Disc previous = beforeMove_.discAt(row, col);
        const int step = (std::min)(
            elapsedMs / FlipFrameDurationMs,
            FlipFrameCount - 1
        );

        if (previous == Disc::Black && current == Disc::White) {
            return StoneBlackToWhiteFirst + step;
        }
        if (previous == Disc::White && current == Disc::Black) {
            return StoneWhiteToBlackFirst + step;
        }
    }

    if (current == Disc::Black) return StoneBlack;
    if (current == Disc::White) return StoneWhite;
    return StoneEmpty;
}

void BoardView::Draw(
    const BitBoard& board,
    BitBoard::Bits legalMoves,
    bool showLegalMoves
) const {
    if (!loaded_) return;

    const auto& images = ImageManager::GetInstance();
    const int gradientBase = (GetNowCount() / 90) % BoardFrameCount;
    const int animationElapsed = animatedBits_ == 0
        ? FlipFrameCount * FlipFrameDurationMs
        : GetNowCount() - animationStartedAt_;

    for (int row = 0; row < BoardSize; ++row) {
        for (int col = 0; col < BoardSize; ++col) {
            const float x = static_cast<float>(BoardLeft + col * CellSize);
            const float y = static_cast<float>(BoardTop + row * CellSize);
            const int boardFrame =
                (gradientBase + row + col) % BoardFrameCount;

            images.Draw(x, y, ImageID::Board, boardFrame, false);

            const int stoneFrame = stoneFrameAt(
                board,
                row,
                col,
                animationElapsed
            );
            if (stoneFrame != StoneEmpty) {
                images.Draw(x, y, ImageID::Stone, stoneFrame);
            } else if (
                showLegalMoves &&
                (legalMoves & BitBoard::bitAt(row, col)) != 0
            ) {
                images.Draw(x, y, ImageID::Stone, StoneLegalMove);
            }

            if (row == lastMoveRow_ && col == lastMoveCol_) {
                images.Draw(x, y, ImageID::Stone, StoneLastMove);
            }
        }
    }

    const float left = static_cast<float>(BoardLeft - CellSize);
    const float right = static_cast<float>(BoardLeft + BoardSize * CellSize);
    const float top = static_cast<float>(BoardTop - CellSize);
    const float bottom = static_cast<float>(BoardTop + BoardSize * CellSize);

    images.Draw(left, top, ImageID::Frame, FrameTopLeft);
    images.Draw(right, top, ImageID::Frame, FrameTopRight);
    images.Draw(left, bottom, ImageID::Frame, FrameBottomLeft);
    images.Draw(right, bottom, ImageID::Frame, FrameBottomRight);

    for (int i = 0; i < BoardSize; ++i) {
        const float x = static_cast<float>(BoardLeft + i * CellSize);
        const float y = static_cast<float>(BoardTop + i * CellSize);
        images.Draw(x, top, ImageID::Frame, FrameTop);
        images.Draw(x, bottom, ImageID::Frame, FrameBottom);
        images.Draw(left, y, ImageID::Frame, FrameLeft);
        images.Draw(right, y, ImageID::Frame, FrameRight);
    }
}

bool BoardView::HitTest(
    int mouseX,
    int mouseY,
    int& row,
    int& col
) const noexcept {
    const int right = BoardLeft + CellSize * BoardSize;
    const int bottom = BoardTop + CellSize * BoardSize;

    if (
        mouseX < BoardLeft || right <= mouseX ||
        mouseY < BoardTop || bottom <= mouseY
    ) {
        return false;
    }

    col = (mouseX - BoardLeft) / CellSize;
    row = (mouseY - BoardTop) / CellSize;
    return true;
}
