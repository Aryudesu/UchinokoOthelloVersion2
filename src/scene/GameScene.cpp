#include "scene/GameScene.h"
#include "manager/InputManager.h"
#include "DxLib.h"

#include <cstdio>

void GameScene::Start() {
    end_ = false;
    next_ = SceneID::Game;
    board_.reset();
}

void GameScene::End() {
}

void GameScene::Update() {
    auto& input = InputManager::GetInstance();

    if (input.isPressed(KEY_INPUT_ESCAPE)) {
        next_ = SceneID::Title;
        end_ = true;
    }
}

void GameScene::Draw() {
    const int boardRight = BoardLeft + CellSize * BoardSize;
    const int boardBottom = BoardTop + CellSize * BoardSize;
    const int boardColor = GetColor(0, 120, 0);
    const int lineColor = GetColor(0, 0, 0);
    const int blackColor = GetColor(0, 0, 0);
    const int whiteColor = GetColor(255, 255, 255);
    const int legalMoveColor = GetColor(160, 220, 160);

    DrawBox(BoardLeft, BoardTop, boardRight, boardBottom, boardColor, TRUE);

    for (int i = 0; i <= BoardSize; ++i) {
        const int x = BoardLeft + i * CellSize;
        const int y = BoardTop + i * CellSize;
        DrawLine(x, BoardTop, x, boardBottom, lineColor);
        DrawLine(BoardLeft, y, boardRight, y, lineColor);
    }

    const BitBoard::Bits legalMoves = board_.legalMoves(Disc::Black);

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

    char score[64];
    std::snprintf(
        score,
        sizeof(score),
        "Black: %d  White: %d",
        board_.count(Disc::Black),
        board_.count(Disc::White)
    );

    DrawString(32, 32, "Game Scene", whiteColor);
    DrawString(32, 56, "ESC : Back to Title", whiteColor);
    DrawString(32, 80, score, whiteColor);
}

bool GameScene::IsEnd() const {
    return end_;
}

SceneID GameScene::NextScene() const {
    return next_;
}
