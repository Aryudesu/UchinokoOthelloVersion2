#pragma once
#include "scene/SceneBase.h"
#include "core/Ids.h"
#include "model/BitBoard.h"

class GameScene : public SceneBase {
    bool end_ = false;
    SceneID next_ = SceneID::Game;
    BitBoard board_;
    Disc turn_ = Disc::Black;
    Disc passed_ = Disc::Empty;
    bool gameOver_ = false;
    bool mouseLeftDown_ = false;

    static constexpr int BoardSize = BitBoard::Size;
    static constexpr int CellSize = 32;
    static constexpr int BoardLeft = 200;
    static constexpr int BoardTop = 80;

    void handleBoardClick();
    void advanceTurn();

public:
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() const override;
};
