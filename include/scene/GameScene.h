#pragma once
#include "scene/SceneBase.h"
#include "core/Ids.h"

class GameScene : public SceneBase {
    bool    end_ = false;
    SceneID next_ = SceneID::Game;

    // 盤面描画用パラメータ
    static constexpr int BoardSize = 8;
    static constexpr int CellSize = 32;   // 1マスのピクセルサイズ（好みで調整）
    static constexpr int BoardLeft = 200;  // 左上位置
    static constexpr int BoardTop = 80;

public:
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() const override;
};
