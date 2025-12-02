#pragma once
#include "SceneBase.h"
#include "Ids.h"
#include "DxLib.h"

class GameScene : public SceneBase {
    bool end_ = false;
    SceneID next_ = SceneID::Game;

public:
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() override;
};
