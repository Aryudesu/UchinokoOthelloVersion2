#include "scene/GameScene.h"
#include "manager/InputManager.h"
#include "DxLib.h"

void GameScene::Start() {
    end_ = false;
    next_ = SceneID::Game;
}

void GameScene::End() {
    // 今は特に後処理なし
}

void GameScene::Update() {
    auto& input = InputManager::GetInstance();

    // とりあえず ESC でタイトルに戻るようにしておく
    if (input.isPressed(KEY_INPUT_ESCAPE)) {
        next_ = SceneID::Title;
        end_ = true;
        return;
    }

    // まだ石を置く処理などは書かない
}

void GameScene::Draw() {
    // 背景色（好みで）
    // ClearDrawScreen はメインループ側でやっているのでここではやらなくてOK

    // 盤面の背景
    int boardLeft = BoardLeft;
    int boardTop = BoardTop;
    int boardRight = BoardLeft + CellSize * BoardSize;
    int boardBottom = BoardTop + CellSize * BoardSize;

    int boardColor = GetColor(0, 120, 0);    // 緑
    int lineColor = GetColor(0, 0, 0);      // 黒線

    // 盤面のベース（塗りつぶし）
    DrawBox(boardLeft, boardTop, boardRight, boardBottom, boardColor, TRUE);

    // 罫線（縦横）
    for (int i = 0; i <= BoardSize; ++i) {
        int x = boardLeft + i * CellSize;
        DrawLine(x, boardTop, x, boardBottom, lineColor);

        int y = boardTop + i * CellSize;
        DrawLine(boardLeft, y, boardRight, y, lineColor);
    }

    // 初期配置の4つの石だけ描画してみる
    int blackColor = GetColor(0, 0, 0);
    int whiteColor = GetColor(255, 255, 255);

    auto drawStone = [&](int row, int col, int color) {
        // row, col は 0 始まりで [0,7]
        float cx = boardLeft + (col + 0.5f) * CellSize;
        float cy = boardTop + (row + 0.5f) * CellSize;
        float r = CellSize * 0.4f;  // 半径

        DrawCircleAA(cx, cy, r, 32, color, TRUE);
        };

    // 標準的な初期配置（黒: (3,4)(4,3), 白: (3,3)(4,4) とかいろいろ流儀あるけど適当に）
    drawStone(3, 3, whiteColor);
    drawStone(3, 4, blackColor);
    drawStone(4, 3, blackColor);
    drawStone(4, 4, whiteColor);

    // UI文字
    DrawString(32, 32, "Game Scene", GetColor(255, 255, 255));
    DrawString(32, 56, "ESC : Back to Title", GetColor(255, 255, 255));
}

bool GameScene::IsEnd() const {
    return end_;
}

SceneID GameScene::NextScene() const {
    return next_;
}
