#pragma once
#include "scene/SceneBase.h"
#include "core/Ids.h"
#include "model/BitBoard.h"

class GameScene : public SceneBase {
    bool end_ = false;
    SceneID next_ = SceneID::Game;
    BitBoard board_;

    static constexpr int BoardSize = BitBoard::Size;
    static constexpr int CellSize = 32;
    static constexpr int BoardLeft = 200;
    static constexpr int BoardTop = 80;

public:
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() const override;
};
