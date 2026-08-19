#pragma once
#include "core/Ids.h"
#include "scene/SceneBase.h"
#include "DxLib.h"

class TitleScene : public SceneBase {
    bool end_ = false;
    SceneID next_ = SceneID::Title;
    int selectedRow_ = 0;

public:
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() const override;
};
