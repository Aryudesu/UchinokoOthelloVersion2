#include "scene/GameScene.h"
#include "manager/InputManager.h"
#include "DxLib.h"

#include <cstdio>

namespace {
    Disc opponentOf(Disc disc) {
        return disc == Disc::Black ? Disc::White : Disc::Black;
    }

    const char* discName(Disc disc) {
        if (disc == Disc::Black) return "Black";
        if (disc == Disc::White) return "White";
        return "";
    }
}

void GameScene::Start() {
    end_ = false;
    next_ = SceneID::Game;
    board_.reset();
    turn_ = Disc::Black;
    passed_ = Disc::Empty;
    gameOver_ = false;
    mouseLeftDown_ = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
}

void GameScene::End() {
}

void GameScene::Update() {
    auto& input = InputManager::GetInstance();

    if (input.isPressed(KEY_INPUT_ESCAPE)) {
        next_ = SceneID::Title;
        end_ = true;
        return;
    }

    const bool mouseLeft = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    const bool clicked = mouseLeft && !mouseLeftDown_;
    mouseLeftDown_ = mouseLeft;

    if (!gameOver_ && clicked) {
        handleBoardClick();
    }
}

void GameScene::handleBoardClick() {
    int mouseX = 0;
    int mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);

    const int boardRight = BoardLeft + CellSize * BoardSize;
    const int boardBottom = BoardTop + CellSize * BoardSize;
    if (mouseX < BoardLeft || boardRight <= mouseX ||
        mouseY < BoardTop || boardBottom <= mouseY) {
        return;
    }

    const int col = (mouseX - BoardLeft) / CellSize;
    const int row = (mouseY - BoardTop) / CellSize;

    if (board_.put(turn_, row, col)) {
        advanceTurn();
    }
}

void GameScene::advanceTurn() {
    const Disc next = opponentOf(turn_);
    passed_ = Disc::Empty;

    if (board_.hasAnyMove(next)) {
        turn_ = next;
        return;
    }

    if (board_.hasAnyMove(turn_)) {
        passed_ = next;
        return;
    }

    gameOver_ = true;
}

void GameScene::Draw() {
    const int boardRight = BoardLeft + CellSize * BoardSize;
    const int boardBottom = BoardTop + CellSize * BoardSize;
    const int boardColor = GetColor(0, 120, 0);
    const int lineColor = GetColor(0, 0, 0);
    const int blackColor = GetColor(0, 0, 0);
    const int whiteColor = GetColor(255, 255, 255);
    const int legalMoveColor = GetColor(160, 220, 160);
    const int noticeColor = GetColor(255, 220, 80);

    DrawBox(BoardLeft, BoardTop, boardRight, boardBottom, boardColor, TRUE);

    for (int i = 0; i <= BoardSize; ++i) {
        const int x = BoardLeft + i * CellSize;
        const int y = BoardTop + i * CellSize;
        DrawLine(x, BoardTop, x, boardBottom, lineColor);
        DrawLine(BoardLeft, y, boardRight, y, lineColor);
    }

    const BitBoard::Bits legalMoves = gameOver_ ? 0 : board_.legalMoves(turn_);

    for (int row = 0; row < BoardSize; ++row) {
        for (int col = 0; col < BoardSize; ++col) {
            const float centerX = BoardLeft + (col + 0.5f) * CellSize;
            const float centerY = BoardTop + (row + 0.5f) * CellSize;
            const Disc disc = board_.discAt(row, col);

            if (disc == Disc::Black) {
                DrawCircleAA(centerX, centerY, CellSize * 0.4f, 32, blackColor, TRUE);
            } else if (disc == Disc::White) {
                DrawCircleAA(centerX, centerY, CellSize * 0.4f, 32, whiteColor, TRUE);
            } else if ((legalMoves & BitBoard::bitAt(row, col)) != 0) {
                DrawCircleAA(centerX, centerY, CellSize * 0.12f, 16, legalMoveColor, TRUE);
            }
        }
    }

    const int blackCount = board_.count(Disc::Black);
    const int whiteCount = board_.count(Disc::White);

    char score[64];
    std::snprintf(score, sizeof(score), "Black: %d  White: %d", blackCount, whiteCount);

    char status[64];
    if (gameOver_) {
        if (blackCount > whiteCount) {
            std::snprintf(status, sizeof(status), "Game Over: Black wins");
        } else if (whiteCount > blackCount) {
            std::snprintf(status, sizeof(status), "Game Over: White wins");
        } else {
            std::snprintf(status, sizeof(status), "Game Over: Draw");
        }
    } else {
        std::snprintf(status, sizeof(status), "Turn: %s", discName(turn_));
    }

    DrawString(32, 32, "Game Scene", whiteColor);
    DrawString(32, 56, "Click a legal move / ESC : Back to Title", whiteColor);
    DrawString(32, 80, score, whiteColor);
    DrawString(32, 104, status, gameOver_ ? noticeColor : whiteColor);

    if (passed_ != Disc::Empty) {
        char passMessage[64];
        std::snprintf(passMessage, sizeof(passMessage), "%s passes", discName(passed_));
        DrawString(32, 128, passMessage, noticeColor);
    }
}

bool GameScene::IsEnd() const {
    return end_;
}

SceneID GameScene::NextScene() const {
    return next_;
}
